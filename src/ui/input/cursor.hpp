#pragma once

#include <cstdint>

namespace xel::ui
{
// 指针光标形状：UI 侧的概念。平台/窗口库只在 UiHost 里做一次映射，
// 控件通过 Node::cursor_at() 表达「指针在我这里该显示什么」。
enum class CursorShape : std::uint8_t
{
    Arrow,
    ResizeEW,   // 左右
    ResizeNS,   // 上下
    ResizeNWSE, // 左上 ↔ 右下
    ResizeNESW, // 右上 ↔ 左下
};
} // namespace xel::ui
