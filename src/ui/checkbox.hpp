#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/box.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>


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
        // 像素对齐：外框、边框与勾选块都落在整像素上，
        // 否则半像素边缘会让勾选块看上去偏心（左右/上下留白不均）。
        const float box = std::round(t.checkbox_size);
        const float bx = std::round(global_.pos.x);
        const float by = std::round(global_.pos.y + (global_.size.y - box) * 0.5f);
        const BoxStyle control = visual_style(StyleRole::Control, control_state(enabled_, is_hovered(), pressed_));
        const glm::vec4 box_bounds{bx, by, box, box};
        const float radius = std::round(corner_radius(t.radius_small));

        graphics::append_rounded_rect(list, box_bounds, radius, t.checkbox_bg);
        // 边框：hover 时用强调色提示可点，平时用 checkbox 专属边框色。
        const glm::vec4 border_color = is_hovered() && enabled_ ? t.accent : t.checkbox_border;
        graphics::append_rounded_ring(list, box_bounds, radius, t.border, border_color);

        if (checked_) {
            // inset 取整：勾选块与外框同心，四边留白完全相等且边界落在整像素。
            const float inset = std::round(box * 0.25f);
            const float marker = box - inset * 2.0f;
            const BoxStyle accent = visual_style(StyleRole::Accent, control_state(enabled_, is_hovered(), pressed_));
            graphics::append_rounded_rect(list,
                                          {bx + inset, by + inset, marker, marker},
                                          radius * 0.5f,
                                          accent.fill);
        }

        append_focus_ring(list, box_bounds, radius);

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, control.text_size);
            const float tx = bx + box + t.spacing;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(list, tx, ty, control.text_size, label_, control.text);
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_leave() override { pressed_ = false; }
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
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (enabled_ && (key == event::KeyCode::Space || key == event::KeyCode::Enter || key == event::KeyCode::KeypadEnter)) {
            checked_ = !checked_;
            if (handler_) handler_(checked_);
            return true;
        }
        return false;
    }
    void on_focus_lost() override { pressed_ = false; Widget::on_focus_lost(); }

private:
    std::string label_;
    bool checked_ = false;
    bool enabled_ = true;
    bool pressed_ = false;
    std::function<void(bool)> handler_;
};
} // namespace xel::ui
