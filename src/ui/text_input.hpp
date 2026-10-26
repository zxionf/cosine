#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/utf8.hpp"
#include "../graphics/box.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace xel::ui
{
class TextInput : public Widget
{
public:
    explicit TextInput(std::string text = {}, float preferred_width = 220.0f)
        : text_{std::move(text)}, preferred_width_{preferred_width}
    {
        cursor_ = text_.size();
        anchor_ = cursor_;
    }

    void set_text(std::string text)
    {
        text_ = std::move(text);
        cursor_ = text_.size();
        anchor_ = cursor_;
        scroll_y_ = 0.0f;
        notify();
        mark_layout_dirty();
    }

    const std::string& text() const { return text_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_on_change(std::function<void(const std::string&)> handler) { handler_ = std::move(handler); }

    // 多行模式：Enter 换行、Up/Down 跨行、Home/End 行内跳转。
    void set_multiline(bool multiline)
    {
        multiline_ = multiline;
        if (!multiline_) {
            // 切回单行时把换行折叠成空格，避免出现无法导航的隐藏行。
            std::replace(text_.begin(), text_.end(), '\n', ' ');
            cursor_ = std::min(cursor_, text_.size());
            anchor_ = std::min(anchor_, text_.size());
            scroll_y_ = 0.0f;
        }
        mark_layout_dirty();
    }
    bool multiline() const { return multiline_; }

    // 多行模式的高度上限（0 表示不限，按行数增长）。
    void set_preferred_size(glm::vec2 size)
    {
        preferred_width_ = size.x;
        max_height_ = size.y;
        mark_layout_dirty();
    }
    void set_max_height(float height)
    {
        max_height_ = height;
        mark_layout_dirty();
    }

    std::size_t cursor() const { return cursor_; }
    std::size_t selection_begin() const { return std::min(cursor_, anchor_); }
    std::size_t selection_end() const { return std::max(cursor_, anchor_); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        if (!multiline_) return {preferred_width_, t.control_height};

        const float line = line_height();
        const float content = static_cast<float>(line_count()) * line;
        float height = content + t.padding * 2.0f;
        if (max_height_ > 0.0f) height = std::min(height, max_height_);
        return {preferred_width_, std::max(height, line + t.padding * 2.0f)};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        clamp_scroll();
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float font_size = t.text_size;
        const float pad = t.padding;
        const float line = line_height();

        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = visual_style(StyleRole::Input, control_state(enabled_, is_hovered(), false));
        const float radius = corner_radius(style.radius);
        graphics::append_rounded_rect(list, bounds, radius, style.fill);

        // 编辑中始终用强调色标出边框（鼠标点击进入编辑也应可见）；
        // 焦点环只在键盘操作期间叠加，两者职责不同。
        const glm::vec4 border_color = is_focused() ? t.accent : style.border;
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, border_color);
        }
        append_focus_ring(list, bounds, radius);

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});

        if (!multiline_) {
            draw_single_line(list, font_size, pad, line);
        } else {
            draw_multi_line(list, font_size, pad, line);
        }

        list.pop_clip();
    }

    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!enabled_) return false;
        cursor_ = index_from_point(pos);
        anchor_ = cursor_;
        dragging_ = true;
        mark_layout_dirty();
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!dragging_) return false;
        cursor_ = index_from_point(pos);
        mark_layout_dirty();
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        dragging_ = false;
        return true;
    }

    bool on_char(char32_t codepoint) override
    {
        if (!enabled_) return false;
        if (codepoint == U'\n') {
            if (!multiline_) return false;
            replace_selection("\n");
            return true;
        }
        if (codepoint < 32 || codepoint == 0x7F) return false;
        replace_selection(graphics::utf8_encode(codepoint));
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier mods) override
    {
        if (!enabled_) return false;

        const bool shift = event::has_modifier(mods, event::Modifier::Shift);
        const bool ctrl = event::has_modifier(mods, event::Modifier::Control);

        if (ctrl && key == event::KeyCode::A) {
            anchor_ = 0;
            cursor_ = text_.size();
            return true;
        }

        switch (key) {
            case event::KeyCode::Backspace:
                if (cursor_ != anchor_) {
                    erase_selection();
                } else if (cursor_ > 0) {
                    const std::size_t prev = graphics::utf8_prev(text_, cursor_);
                    text_.erase(prev, cursor_ - prev);
                    cursor_ = prev;
                    anchor_ = cursor_;
                    notify();
                }
                return true;
            case event::KeyCode::Delete:
                if (cursor_ != anchor_) {
                    erase_selection();
                } else if (cursor_ < text_.size()) {
                    const std::size_t next = graphics::utf8_next(text_, cursor_);
                    text_.erase(cursor_, next - cursor_);
                    notify();
                }
                return true;
            case event::KeyCode::Left:
                cursor_ = graphics::utf8_prev(text_, cursor_);
                if (!shift) anchor_ = cursor_;
                mark_layout_dirty();
                return true;
            case event::KeyCode::Right:
                cursor_ = graphics::utf8_next(text_, cursor_);
                if (!shift) anchor_ = cursor_;
                mark_layout_dirty();
                return true;
            case event::KeyCode::Up:
                if (!multiline_) return false;
                move_vertical(-1, shift);
                return true;
            case event::KeyCode::Down:
                if (!multiline_) return false;
                move_vertical(1, shift);
                return true;
            case event::KeyCode::Home:
                cursor_ = multiline_ ? line_start(line_of(cursor_)) : 0;
                if (!shift) anchor_ = cursor_;
                mark_layout_dirty();
                return true;
            case event::KeyCode::End:
                cursor_ = multiline_ ? line_end(line_of(cursor_)) : text_.size();
                if (!shift) anchor_ = cursor_;
                mark_layout_dirty();
                return true;
            case event::KeyCode::Enter:
            case event::KeyCode::KeypadEnter:
                if (!multiline_) return false;
                replace_selection("\n");
                return true;
            default:
                return false;
        }
    }

    void on_focus_lost() override { dragging_ = false; Widget::on_focus_lost(); }

private:
    float line_height() const
    {
        const float size = ui_theme().text_size;
        if (auto* atlas = font_atlas()) return std::max(atlas->line_height(size), size);
        return size;
    }

    // ---------- 行模型 ----------
    std::size_t line_count() const
    {
        std::size_t count = 1;
        for (char ch : text_) {
            if (ch == '\n') ++count;
        }
        return count;
    }

    std::size_t line_of(std::size_t index) const
    {
        std::size_t line = 0;
        for (std::size_t i = 0; i < std::min(index, text_.size()); ++i) {
            if (text_[i] == '\n') ++line;
        }
        return line;
    }

    std::size_t line_start(std::size_t line) const
    {
        if (line == 0) return 0;
        std::size_t seen = 0;
        for (std::size_t i = 0; i < text_.size(); ++i) {
            if (text_[i] != '\n') continue;
            if (++seen == line) return i + 1;
        }
        return text_.size();
    }

    std::size_t line_end(std::size_t line) const
    {
        std::size_t current = 0;
        for (std::size_t i = 0; i < text_.size(); ++i) {
            if (text_[i] != '\n') continue;
            if (current == line) return i;
            ++current;
        }
        return text_.size();
    }

    // ---------- 宽度 ----------
    float text_width(std::size_t begin, std::size_t end, float font_size) const
    {
        if (begin >= end) return 0.0f;
        const std::string slice = text_.substr(begin, end - begin);
        if (auto* atlas = font_atlas()) return atlas->measure(slice, font_size).x;
        return static_cast<float>(graphics::utf8_codepoint_count(slice)) * font_size * 0.5f;
    }

    float prefix_width(std::size_t index, float font_size) const
    {
        const std::size_t line = line_of(index);
        return text_width(line_start(line), std::min(index, line_end(line)), font_size);
    }

    // ---------- 滚动 ----------
    float content_height() const { return static_cast<float>(line_count()) * line_height() + ui_theme().padding * 2.0f; }

    float max_scroll_y() const
    {
        if (!multiline_) return 0.0f;
        return std::max(0.0f, content_height() - global_.size.y);
    }

    void clamp_scroll()
    {
        scroll_y_ = std::clamp(scroll_y_, 0.0f, max_scroll_y());
    }

    void ensure_cursor_visible(float line, float line_h)
    {
        if (!multiline_) return;
        const float pad = ui_theme().padding;
        const float top = pad + line * line_h - scroll_y_;
        const float bottom = top + line_h;
        const float view_h = global_.size.y;

        if (top < pad) {
            scroll_y_ = pad + line * line_h - pad;
        } else if (bottom > view_h - pad) {
            scroll_y_ = pad + line * line_h + line_h - (view_h - pad);
        }
        clamp_scroll();
    }

    // ---------- 绘制 ----------
    void draw_single_line(graphics::DrawList& list, float font_size, float pad, float line) const
    {
        const auto& t = ui_theme();
        auto* atlas = font_atlas();
        // 用字体行高而不是字号来垂直居中；不要再 max(pad, ...)，
        // 否则行高小于可用高度时会被推到偏下的位置。
        const float line_h = line_height();
        const float text_y = global_.pos.y + std::max(0.0f, (global_.size.y - line_h) * 0.5f);
        const float viewport_w = std::max(0.0f, global_.size.x - pad * 2.0f);
        const float cursor_px = prefix_width(cursor_, font_size);
        const float total_px = text_width(0, text_.size(), font_size);

        float scroll = 0.0f;
        if (cursor_px > viewport_w) scroll = cursor_px - viewport_w + 1.0f;
        if (cursor_px - scroll < 0.0f) scroll = cursor_px;
        scroll = std::clamp(scroll, 0.0f, std::max(0.0f, total_px - viewport_w));
        scroll_offset_ = scroll;

        if (cursor_ != anchor_) {
            const std::size_t lo = std::min(cursor_, anchor_);
            const std::size_t hi = std::max(cursor_, anchor_);
            const float x0 = text_width(0, lo, font_size);
            const float x1 = text_width(0, hi, font_size);
            graphics::Rectangle selection{
                global_.pos.x + pad + x0 - scroll, text_y,
                std::max(1.0f, x1 - x0), line_h, t.text_input_selection};
            selection.append_to(list);
        }

        if (atlas) {
            atlas->append_text(list, global_.pos.x + pad - scroll, text_y, font_size,
                               text_, enabled_ ? t.text : t.text_disabled,
                               {global_.pos.x + pad, global_.pos.y,
                                global_.pos.x + global_.size.x - pad, global_.pos.y + global_.size.y});
        }

        if (is_focused()) {
            // 光标与选区都以行盒为基准，视觉上与文字居中对齐。
            const float cursor_y = text_y + (line_h - font_size) * 0.5f;
            graphics::Rectangle cursor_bar{
                global_.pos.x + pad + cursor_px - scroll, cursor_y, 1.0f, font_size, t.text_input_cursor};
            cursor_bar.append_to(list);
        }
        (void)line;
    }

    void draw_multi_line(graphics::DrawList& list, float font_size, float pad, float line) const
    {
        const auto& t = ui_theme();
        auto* atlas = font_atlas();
        const std::size_t lines = line_count();
        const std::size_t cursor_line = line_of(cursor_);
        const float cursor_x = prefix_width(cursor_, font_size);

        // 水平滚动只影响当前行，长行超出部分被裁掉。
        const float viewport_w = std::max(0.0f, global_.size.x - pad * 2.0f);
        float hscroll = 0.0f;
        if (cursor_x > viewport_w) hscroll = cursor_x - viewport_w + 1.0f;
        hscroll = std::max(0.0f, hscroll);
        scroll_offset_ = hscroll;

        const std::size_t sel_lo = selection_begin();
        const std::size_t sel_hi = selection_end();

        for (std::size_t row = 0; row < lines; ++row) {
            const float y = global_.pos.y + pad + static_cast<float>(row) * line - scroll_y_;
            if (y + line < global_.pos.y || y > global_.pos.y + global_.size.y) continue;

            const std::size_t begin = line_start(row);
            const std::size_t end = line_end(row);
            const float row_scroll = (row == cursor_line) ? hscroll : 0.0f;

            // 跨行选区：本行内的交集。
            if (sel_lo != sel_hi) {
                const std::size_t lo = std::max(sel_lo, begin);
                const std::size_t hi = std::min(sel_hi, end);
                if (lo < hi) {
                    const float x0 = text_width(begin, lo, font_size);
                    const float x1 = text_width(begin, hi, font_size);
                    graphics::Rectangle selection{
                        global_.pos.x + pad + x0 - row_scroll, y,
                        std::max(1.0f, x1 - x0), line, t.text_input_selection};
                    selection.append_to(list);
                }
            }

            if (atlas && begin < end) {
                const std::string slice = text_.substr(begin, end - begin);
                atlas->append_text(list, global_.pos.x + pad - row_scroll, y, font_size,
                                   slice, enabled_ ? t.text : t.text_disabled,
                                   {global_.pos.x, global_.pos.y,
                                    global_.pos.x + global_.size.x, global_.pos.y + global_.size.y});
            }

            if (is_focused() && row == cursor_line) {
                graphics::Rectangle cursor_bar{
                    global_.pos.x + pad + cursor_x - row_scroll, y, 1.0f, line, t.text_input_cursor};
                cursor_bar.append_to(list);
            }
        }
    }

    // ---------- 命中与导航 ----------
    std::size_t index_from_point(glm::vec2 point) const
    {
        const auto& t = ui_theme();
        const float line = line_height();
        if (!multiline_) {
            return index_from_x_single(point.x);
        }

        const float local_y = point.y - (global_.pos.y + t.padding) + scroll_y_;
        std::size_t row = static_cast<std::size_t>(std::max(0.0f, local_y) / line);
        row = std::min(row, line_count() - 1);

        const std::size_t cursor_line = line_of(cursor_);
        const float row_scroll = (row == cursor_line) ? scroll_offset_ : 0.0f;
        const float local_x = point.x - (global_.pos.x + t.padding) + row_scroll;
        return index_in_line(row, local_x);
    }

    std::size_t index_from_x_single(float x) const
    {
        const auto& t = ui_theme();
        const float local_x = x - (global_.pos.x + t.padding) + scroll_offset_;
        return nearest_index(0, text_.size(), local_x);
    }

    std::size_t index_in_line(std::size_t row, float local_x) const
    {
        return nearest_index(line_start(row), line_end(row), local_x);
    }

    std::size_t nearest_index(std::size_t begin, std::size_t end, float local_x) const
    {
        const auto& t = ui_theme();
        std::size_t best = begin;
        float best_dist = std::numeric_limits<float>::max();
        std::size_t i = begin;
        while (true) {
            const float w = text_width(begin, i, t.text_size);
            const float dist = std::fabs(w - local_x);
            if (dist < best_dist) {
                best_dist = dist;
                best = i;
            }
            if (i >= end) break;
            i = graphics::utf8_next(text_, i);
            if (i > end) break;
        }
        return best;
    }

    void move_vertical(int delta, bool shift)
    {
        const auto& t = ui_theme();
        const std::size_t row = line_of(cursor_);
        const std::size_t last = line_count() - 1;

        // 目标行：clamp 到首/末行。
        std::size_t target = row;
        if (delta < 0) target = row == 0 ? 0 : row - 1;
        if (delta > 0) target = row >= last ? last : row + 1;
        if (target == row) return;

        const float x = prefix_width(cursor_, t.text_size);
        const std::size_t new_index = index_in_line(target, x);
        cursor_ = new_index;
        if (!shift) anchor_ = cursor_;
        ensure_cursor_visible(static_cast<float>(target), line_height());
        mark_layout_dirty();
    }

    void erase_selection()
    {
        const size_t lo = std::min(cursor_, anchor_);
        const size_t hi = std::max(cursor_, anchor_);
        text_.erase(lo, hi - lo);
        cursor_ = lo;
        anchor_ = lo;
        notify();
    }

    void replace_selection(const std::string& insert)
    {
        const size_t lo = std::min(cursor_, anchor_);
        const size_t hi = std::max(cursor_, anchor_);
        text_.erase(lo, hi - lo);
        text_.insert(lo, insert);
        cursor_ = lo + insert.size();
        anchor_ = cursor_;
        notify();
    }

    void notify()
    {
        if (handler_) handler_(text_);
        mark_layout_dirty();
        ensure_cursor_visible(static_cast<float>(line_of(cursor_)), line_height());
    }

    std::string text_;
    size_t cursor_ = 0;
    size_t anchor_ = 0;
    float preferred_width_ = 220.0f;
    float max_height_ = 0.0f;
    bool multiline_ = false;
    bool enabled_ = true;
    bool dragging_ = false;
    mutable float scroll_offset_ = 0.0f;
    mutable float scroll_y_ = 0.0f;
    std::function<void(const std::string&)> handler_;
};
} // namespace xel::ui
