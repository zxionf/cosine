#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/utf8.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>

#include <GLFW/glfw3.h>

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
        notify();
        mark_layout_dirty();
    }

    const std::string& text() const { return text_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_on_change(std::function<void(const std::string&)> handler) { handler_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return {preferred_width_, theme().control_height};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = theme();
        const float font_size = t.text_size;
        const float pad = t.padding;
        const float text_y = global_.pos.y + (global_.size.y - font_size) * 0.5f;

        graphics::Rectangle bg{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y, t.text_input_bg};
        bg.append_to(list);

        const float border = t.border;
        const glm::vec4 border_color = focused_ ? t.focus_ring
            : (hovered_ ? glm::vec4{0.45f, 0.48f, 0.52f, 1.0f} : t.text_input_border);
        graphics::Rectangle top   {global_.pos.x, global_.pos.y, global_.size.x, border, border_color};
        graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - border, global_.size.x, border, border_color};
        graphics::Rectangle left  {global_.pos.x, global_.pos.y, border, global_.size.y, border_color};
        graphics::Rectangle right {global_.pos.x + global_.size.x - border, global_.pos.y, border, global_.size.y, border_color};
        top.append_to(list);
        bottom.append_to(list);
        left.append_to(list);
        right.append_to(list);

        auto* atlas = graphics::active_font_atlas();
        const float viewport_w = std::max(0.0f, global_.size.x - pad * 2.0f);
        const float cursor_px = prefix_width(cursor_, font_size);
        const float total_px = prefix_width(text_.size(), font_size);

        float scroll = 0.0f;
        if (cursor_px > viewport_w) {
            scroll = cursor_px - viewport_w + 1.0f;
        }
        if (cursor_px - scroll < 0.0f) {
            scroll = cursor_px;
        }
        scroll = std::clamp(scroll, 0.0f, std::max(0.0f, total_px - viewport_w));

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});

        if (cursor_ != anchor_) {
            const size_t lo = std::min(cursor_, anchor_);
            const size_t hi = std::max(cursor_, anchor_);
            const float x0 = prefix_width(lo, font_size);
            const float x1 = prefix_width(hi, font_size);
            graphics::Rectangle selection{
                global_.pos.x + pad + x0 - scroll, text_y,
                std::max(1.0f, x1 - x0), font_size,
                t.text_input_selection
            };
            selection.append_to(list);
        }

        scroll_offset_ = scroll;
        if (atlas) {
            atlas->append_text(list, global_.pos.x + pad - scroll, text_y, font_size,
                               text_, enabled_ ? t.text : t.text_disabled,
                               {global_.pos.x + pad, global_.pos.y,
                                global_.pos.x + global_.size.x - pad, global_.pos.y + global_.size.y});
        }

        if (focused_) {
            graphics::Rectangle cursor_bar{
                global_.pos.x + pad + cursor_px - scroll, text_y,
                1.0f, font_size, t.text_input_cursor
            };
            cursor_bar.append_to(list);
        }

        list.pop_clip();
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!enabled_) return false;
        cursor_ = index_from_x(pos.x);
        anchor_ = cursor_;
        dragging_ = true;
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!dragging_) return false;
        cursor_ = index_from_x(pos.x);
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
        if (codepoint < 32 || codepoint == 0x7F) return false;
        replace_selection(graphics::utf8_encode(codepoint));
        return true;
    }

    bool on_key_down(int key, int mods) override
    {
        if (!enabled_) return false;

        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;

        if (ctrl && key == GLFW_KEY_A) {
            anchor_ = 0;
            cursor_ = text_.size();
            return true;
        }

        switch (key) {
            case GLFW_KEY_BACKSPACE:

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
            case GLFW_KEY_DELETE:

        if (cursor_ != anchor_) {
                    erase_selection();
                } else if (cursor_ < text_.size()) {
                    const std::size_t next = graphics::utf8_next(text_, cursor_);
                    text_.erase(cursor_, next - cursor_);
                    notify();
                }
                return true;
            case GLFW_KEY_LEFT:
                cursor_ = graphics::utf8_prev(text_, cursor_);
                if (!shift) anchor_ = cursor_;
                return true;
            case GLFW_KEY_RIGHT:
                cursor_ = graphics::utf8_next(text_, cursor_);
                if (!shift) anchor_ = cursor_;
                return true;
            case GLFW_KEY_HOME:
                cursor_ = 0;
                if (!shift) anchor_ = cursor_;
                return true;
            case GLFW_KEY_END:
                cursor_ = text_.size();
                if (!shift) anchor_ = cursor_;
                return true;
            default:
                return false;
        }
    }

    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; dragging_ = false; }

private:
    float prefix_width(size_t count, float font_size) const
    {
        const std::string prefix = text_.substr(0, count);
        if (auto* atlas = graphics::active_font_atlas()) {
            return atlas->measure(prefix, font_size).x;
        }
        return static_cast<float>(graphics::utf8_codepoint_count(prefix)) * font_size * 0.5f;
    }

    size_t index_from_x(float x) const
    {
        const auto& t = theme();
        const float local_x = x - (global_.pos.x + t.padding) + scroll_offset_;
        size_t best = 0;
        float best_dist = std::numeric_limits<float>::max();

        std::size_t i = 0;
        while (true) {
            const float w = prefix_width(i, t.text_size);
            const float dist = std::fabs(w - local_x);
            if (dist < best_dist) {
                best_dist = dist;
                best = i;
            }
            if (i >= text_.size()) break;
            i = graphics::utf8_next(text_, i);
        }
        return best;
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
    }

    std::string text_;
    size_t cursor_ = 0;
    size_t anchor_ = 0;
    float preferred_width_ = 220.0f;
    bool enabled_ = true;
    bool focused_ = false;
    bool dragging_ = false;
    mutable float scroll_offset_ = 0.0f;
    bool hovered_ = false;
    std::function<void(const std::string&)> handler_;
};
} // namespace xel::ui
