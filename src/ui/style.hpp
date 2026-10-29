#pragma once

#include "ui/theme.hpp"

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

// 状态过渡插值：只混合颜色，几何字段（radius / border_width / text_size）直接取目标值。
// 圆角半径变化会改变圆弧分段数，相邻帧顶点数不同会让批次重切。
inline BoxStyle lerp_style(const BoxStyle& from, const BoxStyle& to, float t)
{
    const float k = std::clamp(t, 0.0f, 1.0f);
    BoxStyle style = to;
    style.fill = mix_color(from.fill, to.fill, k);
    style.border = mix_color(from.border, to.border, k);
    style.text = mix_color(from.text, to.text, k);
    style.focus_ring = mix_color(from.focus_ring, to.focus_ring, k);
    return style;
}

// 缓动：ease-out quad，起手快、收尾稳（UI 反馈的直觉）。
inline float ease_out_quad(float t)
{
    const float k = 1.0f - std::clamp(t, 0.0f, 1.0f);
    return 1.0f - k * k;
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
            // 蓝底上用深色字：比蓝字清晰，也把 accent_text 留给「底色上的蓝字」。
            s.text = srgb_hex(0x0F1419);
            s.radius = radius_small;
            break;

        case StyleRole::Row:
            // 行高亮沿用旧观感（button 色系）：选中态由控件自己用 accent 半透明叠加。
            switch (state) {
                case ControlState::Hovered: s.fill = button_hover; break;
                case ControlState::Pressed: s.fill = button_pressed; break;
                default: break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::MenuItem:
            switch (state) {
                case ControlState::Hovered: s.fill = button_hover; break;
                case ControlState::Pressed: s.fill = button_pressed; break;
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
