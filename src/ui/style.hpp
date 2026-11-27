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
// 焦点环不在这里插值：`append_focus_ring` 直接读主题 accent（BoxStyle 不再携带它）。
inline BoxStyle lerp_style(const BoxStyle& from, const BoxStyle& to, float t)
{
    const float k = std::clamp(t, 0.0f, 1.0f);
    BoxStyle style = to;
    style.fill = mix_color(from.fill, to.fill, k);
    style.border = mix_color(from.border, to.border, k);
    style.text = mix_color(from.text, to.text, k);
    return style;
}

// 缓动：ease-out quad，起手快、收尾稳（UI 反馈的直觉）。
inline float ease_out_quad(float t)
{
    const float k = 1.0f - std::clamp(t, 0.0f, 1.0f);
    return 1.0f - k * k;
}

// 交互状态 → 样式状态：唯一入口已移到 Interaction::state()（UI-52）。
// 保留这段注释是为了让后来者明白“为什么这里不应再出现 if 链”。

inline BoxStyle Theme::style(StyleRole role, ControlState state) const
{
    BoxStyle s;
    s.text = state == ControlState::Disabled ? text_disabled : text;
    s.text_size = text_size;
    s.text_size_role = TextSize::Body;
    // 字号档位 → 具体像素值（UI-41）：角色在下面设 text_size_role，这里统一换算。
    const auto apply_text_size = [this, &s]() { s.text_size = text_size_of(s.text_size_role); };

    switch (role) {
        case StyleRole::Surface:
            // hover 时用强调色描边提示可交互（与迁移前的 Panel 行为一致）。
            switch (state) {
                case ControlState::Hovered:
                case ControlState::Checked:  s.border = accent; break;
                case ControlState::Disabled: s.border = panel_border; break;
                default:                     s.border = panel_border; break;
            }
            s.fill = panel_bg;
            s.border_width = border;
            s.radius = radius_panel;
            break;

        case StyleRole::Control:
            switch (state) {
                case ControlState::Disabled: s.fill = button_disabled; break;
                case ControlState::Pressed:  s.fill = button_pressed;  break;
                case ControlState::Hovered:  s.fill = button_hover;    break;
                case ControlState::Checked:  s.fill = accent;          break;
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
                case ControlState::Checked:
                    // Checked = 「正在编辑 / 已展开 / 已勾选」：强调边框。
                    s.fill = text_input_bg;
                    s.border = accent;
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
            // 轨道与滚动条滑块共用：hover 提亮、按下更亮（UI-40 前滑块直读 scrollbar_thumb）。
            switch (state) {
                case ControlState::Disabled: s.fill = button_disabled; break;
                case ControlState::Pressed:  s.fill = accent;         break;
                case ControlState::Hovered:
                case ControlState::Checked:  s.fill = track_hover;    break;
                case ControlState::Normal:   s.fill = slider_track;   break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::Accent:
            s.fill = state == ControlState::Disabled ? text_disabled : accent;
            // accent 上的文字色进主题（UI-39）：之前硬编码的 0x0F1419 在浅色主题下只有 3.8:1。
            s.text = accent_on;
            s.radius = radius_small;
            break;

        case StyleRole::Row:
            // 行高亮沿用旧观感（button 色系）；选中态走独立的 RowSelected 角色。
            switch (state) {
                case ControlState::Hovered: s.fill = button_hover; break;
                case ControlState::Pressed: s.fill = button_pressed; break;
                case ControlState::Disabled: s.fill = button_disabled; break;
                default: break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::RowSelected:
            // 选中行：accent 低透明度叠底（保持旧控件的 accent*0.28 观感）。
            s.fill = with_alpha(accent, 0.28f);
            s.text = text;
            s.radius = radius_small;
            break;

        case StyleRole::MenuItem:
            switch (state) {
                case ControlState::Hovered: s.fill = button_hover; break;
                case ControlState::Pressed: s.fill = button_pressed; break;
                case ControlState::Disabled: s.fill = glm::vec4{0.0f}; break;
                default: break;
            }
            s.radius = radius_small;
            break;

        case StyleRole::Chrome:
            // 窗口壳体（UI-40）：fill 是标题栏底色，border 是外框。
            // Checked 表示「活跃/聚焦」，Hovered 与 Normal 同色（标题栏整体不 hover）。
            switch (state) {
                case ControlState::Disabled: s.fill = panel_bg; s.border = elevated_border; break;
                case ControlState::Checked:
                case ControlState::Pressed:  s.fill = button_hover; s.border = accent; break;
                case ControlState::Hovered:
                case ControlState::Normal:   s.fill = button_hover; s.border = elevated_border; break;
            }
            s.border_width = border;
            s.radius = radius_panel;
            break;

        case StyleRole::Knob:
            // 可拖动手柄：主题级派生（hover/拖动向 accent 靠一档），不再是控件里的 mix_color。
            switch (state) {
                case ControlState::Disabled: s.fill = text_disabled; break;
                case ControlState::Pressed:
                case ControlState::Checked:  s.fill = mix_color(slider_handle, accent, 0.35f); break;
                case ControlState::Hovered:  s.fill = mix_color(slider_handle, accent, 0.15f); break;
                case ControlState::Normal:   s.fill = slider_handle; break;
            }
            s.radius = radius_pill;
            break;

        case StyleRole::Thumb:
            // 滚动条滑块（UI-46）：与 Track 槽分离成独立角色。
            // 之前借用 Track 的 Hovered/Pressed 区分滑块，两者共用一个过渡槽，
            // 每帧互相覆盖 → 滑块颜色被拉平到槽色，看起来“消失”。
            switch (state) {
                case ControlState::Disabled: s.fill = button_disabled; break;
                case ControlState::Pressed:  s.fill = accent;          break;
                case ControlState::Hovered:
                case ControlState::Checked:  s.fill = thumb_hover;     break;
                case ControlState::Normal:   s.fill = thumb;           break;
            }
            s.radius = radius_pill;
            break;

        case StyleRole::Tooltip:
            s.fill = tooltip_bg;
            s.border = panel_border;
            s.border_width = border;
            s.radius = radius_medium;
            s.text_size_role = TextSize::Small; // 浮层用小字
            break;
    }
    apply_text_size();
    return s;
}
} // namespace xel::ui
