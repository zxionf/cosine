#pragma once

#include "theme.hpp"

#include <algorithm>
#include <cmath>

namespace xel::ui
{
// 颜色辅助：样式解析里大量需要「同色不同透明度」与「两色插值」。
inline glm::vec4 with_alpha(glm::vec4 color, float alpha)
{
    color.a = alpha;
    return color;
}

inline glm::vec4 mix_color(glm::vec4 a, glm::vec4 b, float t)
{
    const float k = std::clamp(t, 0.0f, 1.0f);
    return {
        a.r + (b.r - a.r) * k,
        a.g + (b.g - a.g) * k,
        a.b + (b.b - a.b) * k,
        a.a + (b.a - a.a) * k
    };
}

// 唯一的交互状态派生入口：控件不得再自己写 if 链决定用哪套颜色。
inline ControlState control_state(bool enabled, bool hovered, bool pressed)
{
    if (!enabled) return ControlState::Disabled;
    if (pressed)  return ControlState::Pressed;
    if (hovered)  return ControlState::Hovered;
    return ControlState::Normal;
}

inline BoxStyle Theme::style(StyleRole role, ControlState state) const
{
    BoxStyle s;
    s.text = state == ControlState::Disabled ? text_disabled : text;
    s.text_size = text_size;
    s.focus_ring = accent;

    switch (role) {
        case StyleRole::Surface:
            s.fill = panel_bg;
            s.border = panel_border;
            s.border_width = border;
            s.radius = radius_panel;
            break;

        case StyleRole::Control:
            switch (state) {
                case ControlState::Disabled: s.fill = button_disabled; break;
                case ControlState::Pressed:  s.fill = button_pressed;  break;
                case ControlState::Hovered:  s.fill = button_hover;    break;
                case ControlState::Normal:   s.fill = button_bg;       break;
            }
            s.radius = radius;
            break;

        case StyleRole::Input:
            switch (state) {
                case ControlState::Disabled:
                    s.fill = button_disabled;
                    s.border = panel_border;
                    break;
                case ControlState::Pressed:
                case ControlState::Hovered:
                    s.fill = text_input_bg;
                    s.border = text_input_border_hover;
                    break;
                case ControlState::Normal:
                    s.fill = text_input_bg;
                    s.border = text_input_border;
                    break;
            }
            s.border_width = border;
            s.radius = radius;
            break;

        case StyleRole::Track:
            s.fill = state == ControlState::Hovered ? track_hover : slider_track;
            s.radius = radius_small;
            break;

        case StyleRole::Accent:
            s.fill = state == ControlState::Disabled ? text_disabled : accent;
            s.radius = radius_small;
            break;

        case StyleRole::Row:
            switch (state) {
                case ControlState::Hovered: s.fill = with_alpha(accent, 0.12f); break;
                case ControlState::Pressed: s.fill = with_alpha(accent, 0.20f); break;
                default: break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::MenuItem:
            switch (state) {
                case ControlState::Hovered: s.fill = with_alpha(accent, 0.28f); break;
                case ControlState::Pressed: s.fill = with_alpha(accent, 0.40f); break;
                default: break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::Tooltip:
            s.fill = tooltip_bg;
            s.border = panel_border;
            s.border_width = border;
            s.radius = radius_medium;
            break;
    }
    return s;
}
} // namespace xel::ui
