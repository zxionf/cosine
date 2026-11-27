#pragma once

#include "events/event.hpp"
#include "graphics/box.hpp"
#include "ui/scroll_state.hpp"
#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/leaf_node.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>

namespace xel::ui
{
// 行列表的行为基元：滚动、选中、键盘、可见行计算。
// 子类只实现「有多少行」和「怎么画一行」，多列/缩进/文件系统语义都留在子类。
class ListView : public LeafNode
{
public:
    // 语义：列表类（Table / TreeView 会覆写为更具体的 role）。
    WidgetRole semantics_role() const override { return WidgetRole::List; }
    void set_row_height(float height)
    {
        row_height_ = std::max(12.0f, height);
        mark_layout_dirty();
    }
    float row_height() const { return row_height_; }

    int selected() const { return selected_; }
    void set_selected(int index, bool notify = true)
    {
        const int count = static_cast<int>(item_count());
        if (count <= 0) {
            selected_ = -1;
            return;
        }
        const int clamped = std::clamp(index, 0, count - 1);
        if (clamped == selected_) return;
        selected_ = clamped;
        ensure_selected_visible();
        if (notify && on_select_) on_select_(selected_);
    }
    void set_on_select(std::function<void(int)> handler) { on_select_ = std::move(handler); }

    void set_scroll_mode(ScrollMode mode) { scroll_.set_mode(mode); }
    void set_scroll_smoothing(float seconds) { scroll_.set_smoothing(seconds); }
    float scroll_offset() const { return scroll_.offset(); }

    // 子类实现：条目总数与单行绘制（背景/斑马纹由子类决定，基类不介入）。
    virtual std::size_t item_count() const = 0;
    virtual void draw_row(graphics::DrawList& list, std::size_t index, Rect row,
                          bool selected) const = 0;

    bool focusable() const override { return true; }

    // 视口高度：默认整块控件高度；带表头的子类（Table）覆写为内容区高度。
    virtual float viewport_height() const { return global_.size.y; }

    // 基类不直接绘制：子类在 append_to 里用 draw_row 画自己的行。
    // （Widget 继承自 Drawable，append_to 是纯虚，这里必须给一个空实现）
    void append_to(graphics::DrawList&) const override {}

    // 子类决定尺寸来源（Table 用 min_size，TreeView 用 min/max）。
    // 默认（首选尺寸 + clamp）由 LeafNode 提供（UI-47）。

    bool on_scroll(glm::vec2 pos, glm::vec2 delta) override
    {
        if (!global_.contains(pos)) return false;
        if (!scrollable()) return false;
        sync_scroll_range();
        scroll_.scroll_by(-delta.y * row_height_ * 1.5f,
                          scroll_.mode() == ScrollMode::Line);
        return true;
    }

    void on_update(float dt) override { scroll_.update(dt); }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (item_count() == 0) return false;
        switch (key) {
        case event::KeyCode::Up:
            set_selected(selected_ < 0 ? 0 : selected_ - 1);
            return true;
        case event::KeyCode::Down:
            set_selected(selected_ + 1);
            return true;
        case event::KeyCode::Home:
            set_selected(0);
            return true;
        case event::KeyCode::End:
            set_selected(static_cast<int>(item_count()) - 1);
            return true;
        default:
            return false;
        }
    }

protected:
    float content_height() const
    {
        return static_cast<float>(item_count()) * row_height_;
    }
    bool scrollable() const { return content_height() > viewport_height(); }

    void sync_scroll_range() { scroll_.set_range(viewport_height(), content_height()); }

    // 像素对齐的滚动偏移：行绘制与命中都用它，避免行停在半像素上多出一条溢出像素。
    float snapped_offset() const { return std::round(scroll_.offset()); }

    // 第一行可见行号（基于 snapped_offset，与绘制一致）。
    std::size_t first_visible_row() const
    {
        if (row_height_ <= 0.0f) return 0;
        const float snap = snapped_offset();
        return static_cast<std::size_t>(std::max(0.0f, snap) / row_height_);
    }
    // 可见行数（多算一行，避免底部露出半行）。
    std::size_t visible_row_count() const
    {
        if (row_height_ <= 0.0f) return 0;
        return static_cast<std::size_t>(viewport_height() / row_height_) + 1;
    }
    // 第 index 行的顶部 y（相对 body_top），与 first_visible_row 共用同一 snap。
    float row_y(std::size_t index, float body_top) const
    {
        return body_top + static_cast<float>(index) * row_height_ - snapped_offset();
    }
    // 命中测试：给定 body_top 与逻辑 y，返回行号（越界 -1）。与绘制共用同一 snap。
    int row_at(float y, float body_top) const
    {
        if (row_height_ <= 0.0f) return -1;
        const float local = y - body_top + snapped_offset();
        const int row = static_cast<int>(std::floor(local / row_height_));
        if (row < 0 || row >= static_cast<int>(item_count())) return -1;
        return row;
    }

    // 统一行绘制循环：子类在 append_to 里画完表头/背景后调用。
    // 循环、可见窗口、像素对齐、越界裁剪都在基类，子类只填 draw_row。
    void append_rows(graphics::DrawList& list, Rect body) const
    {
        for (std::size_t row = first_visible_row(); row < item_count(); ++row) {
            const float y = row_y(row, body.pos.y);
            if (y + row_height_ < body.pos.y) continue;
            if (y > body.pos.y + body.size.y) break;
            draw_row(list, row, Rect{{body.pos.x, y}, {body.size.x, row_height_}},
                     static_cast<int>(row) == selected_);
        }
    }

    void ensure_selected_visible()
    {
        if (selected_ < 0) return;
        if (viewport_height() <= 0.0f) {
            scroll_.jump_to(0.0f);
            return;
        }
        sync_scroll_range();
        const float top = static_cast<float>(selected_) * row_height_;
        const float bottom = top + row_height_;
        float target = scroll_.offset();
        if (top < target) target = top;
        else if (bottom > target + viewport_height()) target = bottom - viewport_height();
        scroll_.jump_to(target);
    }

    // 垂直滚动条：Table/TreeView 共用同一份绘制。body_top 为轨道顶部（表头下方），
    // body_h 为轨道高度。
    void draw_scrollbar(graphics::DrawList& list, float body_top, float body_h, float bar_w) const
    {
        if (bar_w <= 0.0f || body_h <= 0.0f) return;
        const auto& t = ui_theme();
        const float content = content_height();
        const float max_scroll = std::max(0.0f, content - body_h);
        const float track_x = global_.pos.x + global_.size.x - bar_w;

        const float bar_radius = corner_radius(t.radius_pill);
        // UI-40：槽走 Track、滑块走独立的 Thumb 角色（UI-46）。
        // 两者共用 Track 角色时，同帧内两次请求会互相覆盖过渡槽 → 滑块看不见。
        const BoxStyle track_style = t.style(StyleRole::Track, ControlState::Normal);
        graphics::append_rounded_rect(list, {track_x, body_top, bar_w, body_h}, bar_radius, track_style.fill);

        const float ratio = std::min(1.0f, body_h / std::max(1.0f, content));
        const float thumb_h = std::max(t.scrollbar_min_thumb, body_h * ratio);
        const float travel = std::max(0.0f, body_h - thumb_h);
        const float thumb_y = body_top +
                              (max_scroll > 0.0f ? travel * (scroll_.offset() / max_scroll) : 0.0f);

        const BoxStyle thumb_style = visual_style(StyleRole::Thumb, ControlState::Normal);
        graphics::append_rounded_rect(list, {track_x + 1.0f, thumb_y, std::max(1.0f, bar_w - 2.0f), thumb_h},
                                      bar_radius, thumb_style.fill);
    }

    ScrollState scroll_;
    int selected_ = -1;
    float row_height_ = 32.0f;
    std::function<void(int)> on_select_;
};
} // namespace xel::ui
