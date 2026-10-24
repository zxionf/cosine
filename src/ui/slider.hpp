#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

#include <GLFW/glfw3.h>

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
        const auto& t = theme();
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
        const auto& t = theme();
        const float radius = t.slider_handle_radius;
        const float track_x = global_.pos.x + radius;
        const float track_w = std::max(0.0f, global_.size.x - radius * 2.0f);
        const float center_y = global_.pos.y + global_.size.y * 0.5f;

        const float track_y = center_y - t.slider_height * 0.5f;
        const glm::vec4 track_color = hovered_ ? glm::vec4{0.30f, 0.33f, 0.37f, 1.0f} : t.slider_track;
        graphics::Rectangle track{track_x, track_y, track_w, t.slider_height, track_color};
        track.append_to(list);

        const float range = std::max(1e-6f, max_value_ - min_value_);
        const float norm = std::clamp((value_ - min_value_) / range, 0.0f, 1.0f);
        const float fill_w = track_w * norm;
        if (fill_w > 0.0f) {
            graphics::Rectangle fill{track_x, track_y, fill_w, t.slider_height, t.slider_fill};
            fill.append_to(list);
        }

        const float handle_x = track_x + fill_w - radius;
        const float handle_y = center_y - radius;
        graphics::Rectangle handle{handle_x, handle_y, radius * 2.0f, radius * 2.0f,
                                   enabled_ ? t.slider_handle : t.text_disabled};
        handle.append_to(list);

        if (focused_) {
            const float b = 2.0f;
            graphics::Rectangle top   {handle_x, handle_y, radius * 2.0f, b, t.focus_ring};
            graphics::Rectangle bottom{handle_x, handle_y + radius * 2.0f - b, radius * 2.0f, b, t.focus_ring};
            graphics::Rectangle left  {handle_x, handle_y, b, radius * 2.0f, t.focus_ring};
            graphics::Rectangle right {handle_x + radius * 2.0f - b, handle_y, b, radius * 2.0f, t.focus_ring};
            top.append_to(list);
            bottom.append_to(list);
            left.append_to(list);
            right.append_to(list);
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; }
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
    bool on_key_down(int key, int) override
    {
        if (!enabled_) return false;
        const float range = max_value_ - min_value_;
        const float delta = step_ > 0.0f ? step_ : range * 0.01f;
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_DOWN) {
            set_value(value_ - delta);
            notify();
            return true;
        }
        if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_UP) {
            set_value(value_ + delta);
            notify();
            return true;
        }
        if (key == GLFW_KEY_HOME) {
            set_value(min_value_);
            notify();
            return true;
        }
        if (key == GLFW_KEY_END) {
            set_value(max_value_);
            notify();
            return true;
        }
        return false;
    }
    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; }

private:
    void update_from_pos(glm::vec2 pos)
    {
        const auto& t = theme();
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
    bool hovered_ = false;
    bool focused_ = false;
    std::function<void(float)> handler_;
};
} // namespace xel::ui
