#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/box.hpp"

#include <algorithm>
#include <cmath>
#include <functional>


namespace xel::ui
{
class Slider : public Widget
{
public:
    explicit Slider(float width = 180.0f)
        : preferred_width_{width}
    {}

    void set_range(float min_value, float max_value)
    {
        min_value_ = min_value;
        max_value_ = max_value;
        set_value(value_);
    }

    void set_value(float value)
    {
        const float hi = std::max(min_value_, max_value_);
        const float lo = std::min(min_value_, max_value_);
        value_ = std::clamp(value, lo, hi);
        if (step_ > 0.0f) {
            value_ = lo + std::round((value_ - lo) / step_) * step_;
            value_ = std::clamp(value_, lo, hi);
        }
    }

    float value() const { return value_; }
    void set_step(float step) { step_ = step; set_value(value_); }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_on_change(std::function<void(float)> handler) { handler_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        const float height = std::max(t.control_height, t.slider_handle_radius * 2.0f);
        return {preferred_width_, height};
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
        const float radius = t.slider_handle_radius;
        const float track_x = global_.pos.x + radius;
        const float track_w = std::max(0.0f, global_.size.x - radius * 2.0f);
        const float center_y = global_.pos.y + global_.size.y * 0.5f;
        // 胶囊轨道：modern 取半高，classic（radius_pill=0）为直角。
        const float track_radius = std::min(t.radius_pill, t.slider_height * 0.5f);

        const float track_y = center_y - t.slider_height * 0.5f;
        const float draw_radius = corner_radius(t.radius_pill);
        const BoxStyle track_style = visual_style(StyleRole::Track, is_hovered() ? ControlState::Hovered : ControlState::Normal);
        graphics::append_rounded_rect(list, {track_x, track_y, track_w, t.slider_height}, track_radius, track_style.fill);

        const float range = std::max(1e-6f, max_value_ - min_value_);
        const float norm = std::clamp((value_ - min_value_) / range, 0.0f, 1.0f);
        const float fill_w = track_w * norm;
        if (fill_w > 0.0f) {
            const BoxStyle fill_style = visual_style(StyleRole::Accent, control_state(enabled_, false, false));
            graphics::append_rounded_rect(list, {track_x, track_y, fill_w, t.slider_height}, track_radius, fill_style.fill);
        }

        const glm::vec4 handle_bounds{track_x + fill_w - radius, center_y - radius, radius * 2.0f, radius * 2.0f};
        glm::vec4 handle_color = enabled_ ? t.slider_handle : t.text_disabled;
        if (enabled_ && dragging_) {
            handle_color = mix_color(t.slider_handle, t.accent, 0.35f);
        } else if (enabled_ && is_hovered()) {
            handle_color = mix_color(t.slider_handle, t.accent, 0.15f);
        }
        graphics::append_rounded_rect(list, handle_bounds, draw_radius, handle_color);
        append_focus_ring(list, handle_bounds, draw_radius);
    }

    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!enabled_) return false;
        dragging_ = true;
        update_from_pos(pos);
        return true;
    }
    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!dragging_) return false;
        update_from_pos(pos);
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override
    {
        dragging_ = false;
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!enabled_) return false;
        const float range = max_value_ - min_value_;
        const float delta = step_ > 0.0f ? step_ : range * 0.01f;
        if (key == event::KeyCode::Left || key == event::KeyCode::Down) {
            set_value(value_ - delta);
            notify();
            return true;
        }
        if (key == event::KeyCode::Right || key == event::KeyCode::Up) {
            set_value(value_ + delta);
            notify();
            return true;
        }
        if (key == event::KeyCode::Home) {
            set_value(min_value_);
            notify();
            return true;
        }
        if (key == event::KeyCode::End) {
            set_value(max_value_);
            notify();
            return true;
        }
        return false;
    }
    void on_focus_lost() override { dragging_ = false; Widget::on_focus_lost(); }

private:
    void update_from_pos(glm::vec2 pos)
    {
        const auto& t = ui_theme();
        const float radius = t.slider_handle_radius;
        const float track_x = global_.pos.x + radius;
        const float track_w = std::max(1.0f, global_.size.x - radius * 2.0f);
        const float norm = std::clamp((pos.x - track_x) / track_w, 0.0f, 1.0f);
        set_value(min_value_ + norm * (max_value_ - min_value_));
        notify();
    }

    void notify()
    {
        if (handler_) handler_(value_);
    }

    float preferred_width_ = 180.0f;
    float min_value_ = 0.0f;
    float max_value_ = 1.0f;
    float value_ = 0.0f;
    float step_ = 0.0f;
    bool enabled_ = true;
    bool dragging_ = false;
    std::function<void(float)> handler_;
};
} // namespace xel::ui
