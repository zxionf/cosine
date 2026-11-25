#pragma once

#include <glm/glm.hpp>

namespace xel::ui
{
// 布局约束的"无界"表示。
//
// 约定：constraints 的某一轴 >= kUnbounded 视为该轴无界（父级没有给出可用尺寸）。
// 历史上的三个哨兵（1e8 / 1e9 / -1）统一收敛到这里：
//   - 1e8、1e9：都是"无界"，比较时必须用 is_bounded()，不要直接和字面量比；
//   - -1 及任何负值：同样视为无界 —— 父级没给可用空间时，百分比与宽高比都不该参与计算。
//
// 为什么是 1e8：足够大于任何真实视口（logical px），又远小于 float 精度边界，
// 加上/减去 padding 不会丢精度。
inline constexpr float kUnbounded = 1.0e8f;

// 该轴是否给出了可用尺寸。
inline constexpr bool is_bounded(float extent)
{
    return extent >= 0.0f && extent < kUnbounded;
}

inline bool is_bounded(glm::vec2 extent)
{
    return is_bounded(extent.x) && is_bounded(extent.y);
}
} // namespace xel::ui
