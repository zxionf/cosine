#pragma once

#include "ui/widget_style.hpp"

namespace xel::ui
{
// 样式过渡部件（UI-48 搬家 / UI-50 索引化）：
//   - 内嵌（不再是 unique_ptr）：UI-50 把槽压到 8 B/个，整块 12 槽 ≈ 104 B，
//     即使从未调用 visual_style() 也只占这点空间，不值得再为它做惰性堆分配；
//   - 槽里只存状态与进度，颜色在渲染时从 Theme 现算（见 widget_style.hpp 注释）。
struct VisualState
{
    mutable StyleTransition transition;
};
} // namespace xel::ui
