#pragma once

#include "ui/style/style_types.hpp"

namespace xel::ui
{
// 交互状态：一个控件「现在处于什么状态」的唯一所有者。
//
// 为什么把 state() 放在这里：控制状态 → 样式状态的映射只应有一份定义，
// 否则每加一个控件都会再抄一遍优先级顺序。
struct Interaction
{
    bool enabled = true;   // 可交互（禁用 = 灰显 + 不接事件）
    bool hovered = false;  // 指针在本控件范围内（由 UISystem 写入）
    bool pressed = false;  // 主键按下且仍在本控件内
    bool dragging = false; // 正在拖动（Slider / Splitter / TextInput 选择…）
    bool selected = false; // 选中态（列表/页签/单选的语义投影）
    bool focused = false;  // 拥有键盘焦点（由 UISystem 写入）

    // 交互状态 → 样式状态。优先级与 style.hpp 的 control_state() 完全一致：
    //   Disabled > Pressed > Hovered > Checked > Normal
    ControlState state(bool checked = false) const
    {
        if (!enabled) return ControlState::Disabled;
        if (pressed) return ControlState::Pressed;
        if (hovered) return ControlState::Hovered;
        if (checked) return ControlState::Checked;
        return ControlState::Normal;
    }
};
} // namespace xel::ui
