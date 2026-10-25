#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/rectangle.hpp"

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
        const float height = global_.size.y;

        graphics::Rectangle track{global_.pos.x, global_.pos.y, global_.size.x, height, t.slider_track};
        track.append_to(list);

        const float fill_w = global_.size.x * value_;
        if (fill_w > 0.0f) {
            graphics::Rectangle fill{global_.pos.x, global_.pos.y, fill_w, height, t.slider_fill};
            fill.append_to(list);
        }

        graphics::Rectangle border_top{global_.pos.x, global_.pos.y, global_.size.x, t.border, t.panel_border};
        graphics::Rectangle border_bottom{global_.pos.x, global_.pos.y + height - t.border, global_.size.x, t.border, t.panel_border};
        border_top.append_to(list);
        border_bottom.append_to(list);
    }

    bool hit_testable() const override { return false; }

private:
    float value_ = 0.0f;
    float preferred_width_ = 220.0f;
};
} // namespace xel::ui
