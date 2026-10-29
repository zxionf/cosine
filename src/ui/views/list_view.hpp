#pragma once

#include "events/event.hpp"
#include "ui/scroll_state.hpp"
#include "ui/widget.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>

namespace xel::ui
{
// 行列表的行为基元：滚动、选中、键盘、可见行计算。
// 子类只实现「有多少行」和「怎么画一行」，多列/缩进/文件系统语义都留在子类。
class ListView : public Widget
{
public:
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

    // 子类决定尺寸来源（Table 用 min_size，TreeView 可能用父宽）。
    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return clamp_size(local_.size);
    }
    void arrange(Rect slot) override
    {
        set_pos(slot.pos);
        set_size(slot.size);
        global_ = slot;
    }

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

    // 第一行可见行号（按当前滚动偏移）。
    std::size_t first_visible_row() const
    {
        if (row_height_ <= 0.0f) return 0;
        const float row = scroll_.offset() / row_height_;
        return static_cast<std::size_t>(std::max(0.0f, std::floor(row)));
    }
    // 可见行数（多算一行，避免底部露出半行）。
    std::size_t visible_row_count() const
    {
        if (row_height_ <= 0.0f) return 0;
        return static_cast<std::size_t>(viewport_height() / row_height_) + 1;
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

    ScrollState scroll_;
    int selected_ = -1;
    float row_height_ = 32.0f;
    std::function<void(int)> on_select_;
};
} // namespace xel::ui
