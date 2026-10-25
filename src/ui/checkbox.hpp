#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <string>

#include <GLFW/glfw3.h>

namespace xel::ui
{
class Checkbox : public Widget
{
public:
    explicit Checkbox(std::string label, bool checked = false,
                      std::function<void(bool)> handler = {})
        : label_{std::move(label)}, checked_{checked}, handler_{std::move(handler)}
    {}

    void set_checked(bool checked) { checked_ = checked; }
    bool checked() const { return checked_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_on_change(std::function<void(bool)> handler) { handler_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        float text_w = static_cast<float>(label_.size()) * t.text_size * 0.5f;
        if (auto* atlas = font_atlas()) {
            text_w = atlas->measure(label_, t.text_size).x;
        }
        return {
            t.checkbox_size + (label_.empty() ? 0.0f : t.spacing + text_w),
            std::max(t.checkbox_size, t.text_size)
        };
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float box = t.checkbox_size;
        const float bx = global_.pos.x;
        const float by = global_.pos.y + (global_.size.y - box) * 0.5f;

        graphics::Rectangle box_bg{bx, by, box, box, t.checkbox_bg};
        box_bg.append_to(list);

        const glm::vec4 border_color = hovered_ ? t.focus_ring : t.checkbox_border;
        graphics::Rectangle border_top   {bx, by, box, t.border, border_color};
        graphics::Rectangle border_bottom{bx, by + box - t.border, box, t.border, border_color};
        graphics::Rectangle border_left  {bx, by, t.border, box, border_color};
        graphics::Rectangle border_right {bx + box - t.border, by, t.border, box, border_color};
        border_top.append_to(list);
        border_bottom.append_to(list);
        border_left.append_to(list);
        border_right.append_to(list);

        if (checked_) {
            const float inset = box * 0.25f;
            graphics::Rectangle fill{bx + inset, by + inset, box - inset * 2.0f, box - inset * 2.0f, t.checkbox_fill};
            fill.append_to(list);
        }

        if (focused_) {
            const float b = 2.0f;
            graphics::Rectangle top   {bx - b, by - b, box + b * 2.0f, b, t.focus_ring};
            graphics::Rectangle bottom{bx - b, by + box, box + b * 2.0f, b, t.focus_ring};
            top.append_to(list);
            bottom.append_to(list);
        }

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, t.text_size);
            const float tx = bx + box + t.spacing;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(list, tx, ty, t.text_size, label_, enabled_ ? t.text : t.text_disabled);
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; }
    bool on_mouse_down(glm::vec2, int) override
    {
        if (!enabled_) return false;
        pressed_ = true;
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override
    {
        pressed_ = false;
        return true;
    }
    bool on_click(glm::vec2, int) override
    {
        if (!enabled_) return false;
        checked_ = !checked_;
        if (handler_) handler_(checked_);
        return true;
    }
    bool on_key_down(int key, int) override
    {
        if (enabled_ && (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)) {
            checked_ = !checked_;
            if (handler_) handler_(checked_);
            return true;
        }
        return false;
    }
    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; }

private:
    std::string label_;
    bool checked_ = false;
    bool enabled_ = true;
    bool hovered_ = false;
    bool pressed_ = false;
    bool focused_ = false;
    std::function<void(bool)> handler_;
};
} // namespace xel::ui
