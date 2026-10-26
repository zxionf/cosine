#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/box.hpp"

#include <algorithm>

namespace xel::ui
{
// 进度条：0..1 的确定进度，颜色跟随主题。
class Progress : public Widget
{
public:
    explicit Progress(float width = 220.0f) : preferred_width_{width} {}

    void set_value(float value)
    {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        if (clamped == value_) return;
        value_ = clamped;
    }

    float value() const { return value_; }
    void set_preferred_width(float width) { preferred_width_ = width; mark_layout_dirty(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        return {preferred_width_, std::max(t.slider_height * 2.0f, 10.0f)};
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
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const float radius = corner_radius(t.radius_pill);

        const BoxStyle track_style = t.style(StyleRole::Track, ControlState::Normal);
        graphics::append_rounded_rect(list, bounds, radius, track_style.fill);

        const float fill_w = global_.size.x * value_;
        if (fill_w > 0.0f) {
            const BoxStyle fill_style = t.style(StyleRole::Accent, ControlState::Normal);
            graphics::append_rounded_rect(list, {bounds.x, bounds.y, fill_w, bounds.w}, radius, fill_style.fill);
        }

        if (t.border > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, t.border, t.panel_border);
        }
    }

    bool hit_testable() const override { return false; }

private:
    float value_ = 0.0f;
    float preferred_width_ = 220.0f;
};
} // namespace xel::ui
