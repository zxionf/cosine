#pragma once

namespace xel::ui
{
// 样式角色：把控件抽象成「用途」，Theme 按角色而不是控件名提供外观。
// 新控件优先复用已有角色，避免每加一个控件就往 Theme 里塞一整套字段。
enum class StyleRole
{
    Surface,     // 面板 / 卡片底
    Control,     // 按钮类：Button、Slider 手柄、Checkbox/Radio 框体
    Input,       // 输入类框体：TextInput、Dropdown 框
    Track,       // 滑轨 / 进度 / 滚动条槽与滑块
    Accent,      // 选中与强调填充：勾选标记、滑动填充、进度填充
    Row,         // 列表行：Table / TreeView / List
    MenuItem,    // 菜单项
    Tooltip,     // 浮层信息
    Chrome,      // 窗口标题栏等壳体（UI-40）
    RowSelected, // 列表行选中态（UI-40）
    Knob,        // 可拖动手柄（Slider）：高对比中性色，不等于按钮底色（UI-40）
};

// 交互状态：填充只看这一个维度。焦点不是填充状态，焦点环是叠加层，可与 hover 共存。
// 优先级：Disabled > Pressed > Hovered > Checked > Normal。
enum class ControlState
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Checked, // 勾选 / 选中（UI-40）：Checkbox/Radio 的按下状态之外还要表达「已选」
};

// 字号档位（UI-41）：角色表除了颜色/圆角，也能给出「这类控件用多大字」。
// 以前 Tooltip/MenuItem 快捷键/TimelineView 只能直读 small_text_size，角色表无法表达。
enum class TextSize
{
    Body,    // 正文：Theme::text_size
    Small,   // 辅助：Theme::small_text_size（提示/标尺/坐标轴）
    Heading, // 标题：Theme::heading_text_size
};
} // namespace xel::ui
