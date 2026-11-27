#pragma once

#include "ui/style/widget_style.hpp"

namespace xel::ui
{
// 样式过渡部件：内嵌，无堆分配 —— 槽压到 8 B/个，12 槽 ≈ 104 B，
// 不值得为它做惰性分配；槽里只存状态与进度，颜色渲染时从 Theme 现算
// （见 widget_style.hpp）。
struct VisualState
{
    mutable StyleTransition transition;
};
} // namespace xel::ui
