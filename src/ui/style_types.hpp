#pragma once

namespace xel::ui
{
// 样式角色：把控件抽象成「用途」，Theme 按角色而不是控件名提供外观。
// 新控件优先复用已有角色，避免每加一个控件就往 Theme 里塞一整套字段。
enum class StyleRole
{
    Surface,  // 面板 / 卡片底
    Control,  // 按钮类：Button、Checkbox 方框、Slider 手柄
    Input,    // 输入类框体：TextInput、Dropdown 框
    Track,    // 滑轨 / 进度 / 滚动条槽
    Accent,   // 选中与强调填充：勾选、滑动填充、进度填充
    Row,      // 列表行：Table / TreeView / List
    MenuItem, // 菜单项
    Tooltip,  // 浮层信息
};

// 交互状态：填充只看这一个维度。焦点不是填充状态，焦点环是叠加层，可与 hover 共存。
// 优先级：Disabled > Pressed > Hovered > Normal。
enum class ControlState
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
};
} // namespace xel::ui
