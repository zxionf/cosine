#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "graphics/box.hpp"

#include <algorithm>
#include <cmath>

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

    // 不确定态：条带循环滑动，由 on_update(dt) 推进。
    void set_indeterminate(bool indeterminate)
    {
        indeterminate_ = indeterminate;
        mark_layout_dirty();
    }
    bool indeterminate() const { return indeterminate_; }
    float phase() const { return phase_; }

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

        const BoxStyle fill_style = t.style(StyleRole::Accent, ControlState::Normal);
        if (indeterminate_) {
            // 条带保持完整宽度滑出，超出部分交给裁剪；
            // 注意 x 是相对控件左边缘的偏移，必须加上 bounds.x 才是绝对坐标。
            const float band_w = std::max(24.0f, bounds.z * 0.3f);
            const float x = bounds.x + (bounds.z + band_w) * phase_ - band_w;
            list.push_clip(bounds);
            graphics::append_rounded_rect(list, {x, bounds.y, band_w, bounds.w}, radius, fill_style.fill);
            list.pop_clip();
        } else {
            const float fill_w = bounds.z * value_;
            if (fill_w > 0.0f) {
                graphics::append_rounded_rect(list, {bounds.x, bounds.y, fill_w, bounds.w},
                                              radius, fill_style.fill);
            }
        }

        if (t.border > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, t.border, t.panel_border);
        }
    }

    bool hit_testable() const override { return false; }

protected:
    void on_update(float dt) override
    {
        if (!indeterminate_) return;
        constexpr float kPeriod = 1.2f;
        phase_ = std::fmod(phase_ + dt / kPeriod, 1.0f);
        if (phase_ < 0.0f) phase_ += 1.0f;
        // 浮点累积可能得到 0.9999999，归零避免相位卡在末尾。
        if (phase_ >= 1.0f - 1e-4f) phase_ = 0.0f;
    }

public:

private:
    float value_ = 0.0f;
    float preferred_width_ = 220.0f;
    bool indeterminate_ = false;
    float phase_ = 0.0f;
};
} // namespace xel::ui
