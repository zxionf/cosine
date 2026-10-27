#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../events/event.hpp"
#include "../graphics/box.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <string>

namespace xel::ui
{
// 拖动改值的数值框（属性面板用）。
// 左键按住横向拖动改变数值；方向键微调；Home/End 到边界。
// 显示 `label  value`，label 为空时只显示数值。
class DragFloat : public Widget
{
public:
    explicit DragFloat(std::string label = {}, float width = 150.0f)
        : label_(std::move(label)), preferred_width_(width)
    {}

    void set_range(float min_value, float max_value)
    {
        min_value_ = std::min(min_value, max_value);
        max_value_ = std::max(min_value, max_value);
        set_value(value_);
    }

    // 拖动灵敏度：每像素改变多少。<= 0 时按「200 像素走完全程」自动推导。
    void set_step(float step) { step_ = std::max(0.0f, step); }

    void set_precision(int digits) { precision_ = std::clamp(digits, 0, 6); }
    void set_suffix(std::string suffix) { suffix_ = std::move(suffix); }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

    void set_value(float value)
    {
        value_ = std::clamp(value, min_value_, max_value_);
    }
    float value() const { return value_; }

    void set_on_change(std::function<void(float)> handler) { handler_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return {preferred_width_, ui_theme().control_height};
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
        ControlState state = ControlState::Normal;
        if (!enabled_) {
            state = ControlState::Disabled;
        } else if (dragging_) {
            state = ControlState::Pressed;
        } else if (is_hovered()) {
            state = ControlState::Hovered;
        }

        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = visual_style(StyleRole::Input, state);
        const float radius = corner_radius(style.radius);
        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, style.border);
        }

        auto* atlas = font_atlas();
        if (atlas == nullptr) return;

        const float size = t.text_size;
        const float line = atlas->line_height(size);
        const float text_y = global_.pos.y + (global_.size.y - line) * 0.5f;
        const float pad = std::max(4.0f, t.padding * 0.5f);
        const glm::vec4 label_color = enabled_ ? t.text : t.text_disabled;
        const glm::vec4 value_color = !enabled_ ? t.text_disabled
                                     : (dragging_ ? t.accent : t.text);

        float x = global_.pos.x + pad;
        if (!label_.empty()) {
            atlas->append_text(list, x, text_y, size, label_, label_color);
            x += atlas->measure_width(label_, size) + pad;
        }

        const std::string shown = std::format("{:.{}f}{}", value_, precision_, suffix_);
        const float shown_width = atlas->measure_width(shown, size);
        const float shown_x = std::max(x, global_.pos.x + global_.size.x - pad - shown_width);
        atlas->append_text(list, shown_x, text_y, size, shown, value_color);
    }

    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!enabled_) return false;
        dragging_ = true;
        drag_start_x_ = pos.x;
        drag_start_value_ = value_;
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!dragging_) return false;
        const float per_pixel = step_per_pixel();
        set_value(drag_start_value_ + (pos.x - drag_start_x_) * per_pixel);
        notify();
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        const bool was_dragging = dragging_;
        dragging_ = false;
        return was_dragging;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!enabled_) return false;
        const float delta = step_ > 0.0f ? step_
                                         : std::max(1.0e-4f, (max_value_ - min_value_) / 100.0f);
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

    void on_focus_lost() override
    {
        dragging_ = false;
        Widget::on_focus_lost();
    }

private:
    float step_per_pixel() const
    {
        if (step_ > 0.0f) return step_;
        return std::max(1.0e-4f, (max_value_ - min_value_) / 200.0f);
    }

    void notify()
    {
        if (handler_) handler_(value_);
    }

    std::string label_;
    std::string suffix_;
    float preferred_width_ = 150.0f;
    float min_value_ = 0.0f;
    float max_value_ = 1.0f;
    float value_ = 0.0f;
    float step_ = 0.0f;
    int precision_ = 1;
    bool enabled_ = true;
    bool dragging_ = false;
    float drag_start_x_ = 0.0f;
    float drag_start_value_ = 0.0f;
    std::function<void(float)> handler_;
};
} // namespace xel::ui
