#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "ui/views/list_view.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"
#include "graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
struct TableColumn
{
    std::string title;
    // width <= 0 表示弹性列：剩余宽度在弹性列之间均分。
    float width = 120.0f;
    enum class Align { Left, Center, Right };
    Align align = Align::Left;
};

// 表格：固定表头 + 可滚动的内容区，行可选中。
class Table : public ListView
{
public:
    Table(float width = 480.0f, float height = 240.0f)
    {
        set_min_size({width, height});
        set_max_size({width, height});
    }

    void set_columns(std::vector<TableColumn> columns)
    {
        columns_ = std::move(columns);
        mark_layout_dirty();
    }
    const std::vector<TableColumn>& columns() const { return columns_; }

    void set_rows(std::vector<std::vector<std::string>> rows)
    {
        rows_ = std::move(rows);
        if (selected_ >= static_cast<int>(rows_.size())) selected_ = rows_.empty() ? -1 : 0;
        set_scroll_value(0.0f);
        mark_layout_dirty();
    }
    void add_row(std::vector<std::string> row)
    {
        rows_.push_back(std::move(row));
        mark_layout_dirty();
    }
    void clear_rows()
    {
        rows_.clear();
        selected_ = -1;
        set_scroll_value(0.0f);
        mark_layout_dirty();
    }

    std::size_t row_count() const { return rows_.size(); }
    const std::vector<std::vector<std::string>>& rows() const { return rows_; }

    // 选中/行高/滚动/回调均继承自 ListView。

    // 选中/行高/滚动均来自 ListView；Table 只额外拥有表头与多列。
    std::size_t item_count() const override { return rows_.size(); }
    // 视口要去掉表头高度（ListView 默认用 global_.size.y）。
    float viewport_height() const override { return body_height(); }
    // 目前行绘制仍在 append_to 里（带表头/列宽/斑马纹），这里留空以满纯虚契约。
    void draw_row(graphics::DrawList&, std::size_t, Rect, bool) const override {}
    void set_header_height(float height)
    {
        header_height_ = std::max(0.0f, height);
        mark_layout_dirty();
    }
    float header_height() const { return header_height_; }
    void set_striped(bool striped) { striped_ = striped; }
    bool striped() const { return striped_; }

    // 0（默认）= 跟随主题字号；FileBrowser 这类列表会显式指定。
    void set_text_size(float size)
    {
        text_size_ = size > 0.0f ? size : 0.0f;
        mark_layout_dirty();
    }
    float text_size() const { return text_size_; }

    float scroll_offset() const { return scroll_.offset(); }

    // 滚动模式：Smooth = 滚轮只改目标、位置平滑逼近；Line = 立即到位（按行）。
    void set_scroll_mode(ScrollMode mode) { scroll_.set_mode(mode); }
    ScrollMode scroll_mode() const { return scroll_.mode(); }
    void set_scroll_smoothing(float seconds) { scroll_.set_smoothing(seconds); }
    float scroll_smoothing() const { return scroll_.smoothing(); }
    float scroll_target() const { return scroll_.target(); }

    // 立即设置滚动值并同步目标（程序化、键盘、结构变化时用）。
    void set_scroll_value(float value) { scroll_.jump_to(value); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        // 尺寸由 min/max 决定（构造时固定）；不读 global_，避免依赖 arrange 顺序。
        return clamp_size(min_size());
    }
    bool fixed_size() const override { return true; }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        // 视口/内容变化后重新夹取，保留平滑目标（只按新范围收紧）。
        scroll_.set_range(body_height(), content_height());
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();

        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
        const float panel_radius = corner_radius(surface.radius);
        graphics::append_rounded_rect(list, bounds, panel_radius, surface.fill);

        list.push_clip({std::round(global_.pos.x), std::round(global_.pos.y),
                        std::round(global_.size.x), std::round(global_.size.y)});

        const float header_h = header_height_;
        const float body_top_probe = global_.pos.y + header_h;
        const float body_h_probe = std::max(0.0f, global_.size.y - header_h);
        const float bar_w_probe = content_height() > body_h_probe ? t.scrollbar_width + 2.0f : 0.0f;
        const float body_w_probe = std::max(0.0f, global_.size.x - bar_w_probe);
        // 列宽按「去掉滚动条后的正文宽度」分配，否则右对齐的最后一列会被滚动条裁掉。
        const auto widths = column_widths(body_w_probe);

        // 表头
        if (header_h > 0.0f) {
            const BoxStyle header_style = t.style(StyleRole::Control, ControlState::Pressed);
            graphics::Rectangle header{global_.pos.x, global_.pos.y, global_.size.x, header_h,
                                       header_style.fill};
            header.append_to(list);

            float x = global_.pos.x;
            for (std::size_t i = 0; i < columns_.size() && i < widths.size(); ++i) {
                draw_cell(list, columns_[i].title, x, global_.pos.y, widths[i], header_h,
                          columns_[i].align, t.text, x, global_.pos.x + body_w_probe);
                x += widths[i];
            }
            graphics::Rectangle line{global_.pos.x, global_.pos.y + header_h - 1.0f,
                                     global_.size.x, 1.0f, t.panel_border};
            line.append_to(list);
        }

        const float body_top = global_.pos.y + header_h;
        const float body_h = std::max(0.0f, global_.size.y - header_h);
        const float bar_w = content_height() > body_h ? t.scrollbar_width + 2.0f : 0.0f;
        const float body_w = std::max(0.0f, global_.size.x - bar_w);

        list.push_clip({std::round(global_.pos.x), std::round(body_top),
                        std::round(body_w), std::round(body_h)});

        // 像素对齐：滚动偏移取整，避免行停在半像素上而多出一条溢出像素。
        const float snap = std::round(scroll_.offset());
        std::size_t first_visible = row_height_ > 0.0f
            ? static_cast<std::size_t>(std::max(0.0f, snap) / row_height_)
            : 0;
        for (std::size_t row = first_visible; row < rows_.size(); ++row) {
            const float y = body_top + static_cast<float>(row) * row_height_ - snap;
            if (y + row_height_ < body_top) continue;
            if (y > body_top + body_h) break;

            const bool is_selected = static_cast<int>(row) == selected_;
            const bool is_hovered = static_cast<int>(row) == hovered_;

            const float row_radius = corner_radius(t.radius_small);
            if (is_selected) {
                const glm::vec4 color{t.accent.r, t.accent.g, t.accent.b, 0.28f};
                graphics::append_rounded_rect(list, {global_.pos.x + 2.0f, y, body_w - 4.0f, row_height_},
                                              row_radius, color);
            } else if (is_hovered) {
                const BoxStyle row_style = t.style(StyleRole::Row, ControlState::Hovered);
                graphics::append_rounded_rect(list, {global_.pos.x + 2.0f, y, body_w - 4.0f, row_height_},
                                              row_radius, row_style.fill);
            } else if (striped_ && (row % 2) == 1) {
                graphics::append_rounded_rect(list, {global_.pos.x + 2.0f, y, body_w - 4.0f, row_height_},
                                              row_radius, {1.0f, 1.0f, 1.0f, 0.03f});
            }

            float x = global_.pos.x;
            const auto& cells = rows_[row];
            for (std::size_t col = 0; col < widths.size(); ++col) {
                const std::string& text = col < cells.size() ? cells[col] : empty_;
                draw_cell(list, text, x, y, widths[col], row_height_,
                          col < columns_.size() ? columns_[col].align : TableColumn::Align::Left,
                          t.text, x, global_.pos.x + body_w);
                x += widths[col];
            }
        }
        list.pop_clip();

        draw_scrollbar(list, body_top, body_h, bar_w);

        append_focus_ring(list, bounds, panel_radius);

        list.pop_clip();
    }

    bool hit_testable() const override { return true; }
    bool focusable() const override { return true; }
    bool clips_children() const override { return true; }

    void on_mouse_enter() override { hovered_ = hovered_row(last_mouse_); }
    void on_mouse_leave() override { hovered_ = -1; }
    void on_hover(glm::vec2 pos) override
    {
        last_mouse_ = pos;
        hovered_ = hovered_row(pos);
    }

    bool on_click(glm::vec2 pos, int) override
    {
        const int row = row_at(pos);
        if (row < 0) return false;
        set_selected(row);
        return true;
    }

    // 导航键（Up/Down/Home/End）由 ListView 统一处理，Table 没有额外键。

    void on_focus_lost() override
    {
        Widget::on_focus_lost();
    }

private:
    float font_size() const
    {
        const auto& t = ui_theme();
        return text_size_ > 0.0f ? text_size_ : t.text_size;
    }

    float body_height() const { return std::max(0.0f, global_.size.y - header_height_); }
    float content_height() const { return static_cast<float>(rows_.size()) * row_height_; }

    std::vector<float> column_widths(float available_width) const
    {
        std::vector<float> widths;
        widths.reserve(columns_.size());
        float fixed = 0.0f;
        int flex_count = 0;
        for (const auto& column : columns_) {
            if (column.width > 0.0f) {
                widths.push_back(column.width);
                fixed += column.width;
            } else {
                widths.push_back(0.0f);
                ++flex_count;
            }
        }
        if (flex_count > 0) {
            const float remaining = std::max(0.0f, available_width - fixed);
            const float share = remaining / static_cast<float>(flex_count);
            for (auto& width : widths) {
                if (width <= 0.0f) width = share;
            }
        }
        return widths;
    }

    float text_advance(const std::string& text) const
    {
        const auto& t = ui_theme();
        if (auto* atlas = font_atlas()) return atlas->measure(text, font_size()).x;
        return static_cast<float>(text.size()) * font_size() * 0.5f;
    }

    void draw_cell(graphics::DrawList& list, const std::string& text,
                   float x, float y, float width, float height,
                   TableColumn::Align align, glm::vec4 color,
                   float clip_left, float clip_right) const
    {
        auto* atlas = font_atlas();
        if (!atlas || text.empty()) return;

        const auto& t = ui_theme();
        const float pad = t.padding;
        const float text_w = text_advance(text);
        const float available = std::max(0.0f, width - pad * 2.0f);
        float text_x = x + pad;
        switch (align) {
            case TableColumn::Align::Center: text_x = x + (width - text_w) * 0.5f; break;
            case TableColumn::Align::Right: text_x = x + width - pad - text_w; break;
            case TableColumn::Align::Left: break;
        }
        if (text_x < x + pad) text_x = x + pad;
        // 行高居中：用行高而不是字号。
        const float text_y = atlas->centered_text_y(font_size(), y, height);

        list.push_clip({std::max(x + pad * 0.5f, clip_left), y,
                        std::min(available + pad, std::max(0.0f, clip_right - x)), height});
        atlas->append_text(list, text_x, text_y, font_size(), text, color);
        list.pop_clip();
    }

    void draw_scrollbar(graphics::DrawList& list, float body_top, float body_h, float bar_w) const
    {
        if (bar_w <= 0.0f || body_h <= 0.0f) return;
        const auto& t = ui_theme();
        const float content = content_height();
        const float max_scroll = std::max(0.0f, content - body_h);
        const float track_x = global_.pos.x + global_.size.x - bar_w;

        const float bar_radius = corner_radius(t.radius_pill);
        const BoxStyle track_style = t.style(StyleRole::Track, ControlState::Normal);
        graphics::append_rounded_rect(list, {track_x, body_top, bar_w, body_h}, bar_radius, track_style.fill);

        const float ratio = std::min(1.0f, body_h / std::max(1.0f, content));
        const float thumb_h = std::max(t.scrollbar_min_thumb, body_h * ratio);
        const float travel = std::max(0.0f, body_h - thumb_h);
        const float thumb_y = body_top + (max_scroll > 0.0f ? travel * (scroll_.offset() / max_scroll) : 0.0f);

        graphics::append_rounded_rect(list, {track_x + 1.0f, thumb_y, std::max(1.0f, bar_w - 2.0f), thumb_h},
                                      bar_radius, t.scrollbar_thumb);
    }

    int row_at(glm::vec2 pos) const
    {
        const float body_top = global_.pos.y + header_height_;
        if (pos.y < body_top || pos.y > global_.pos.y + global_.size.y) return -1;
        if (pos.x < global_.pos.x || pos.x > global_.pos.x + global_.size.x) return -1;
        const float local_y = pos.y - body_top + scroll_.offset();
        const int row = static_cast<int>(std::floor(local_y / row_height_));
        if (row < 0 || row >= static_cast<int>(rows_.size())) return -1;
        return row;
    }

    int hovered_row(glm::vec2 pos) const { return row_at(pos); }

    std::vector<TableColumn> columns_;
    std::vector<std::vector<std::string>> rows_;
    std::string empty_;
    int hovered_ = -1;
    glm::vec2 last_mouse_{-1.0f, -1.0f};
    float header_height_ = 30.0f;
    bool striped_ = true;
    float text_size_ = 0.0f;
};
} // namespace xel::ui
