#pragma once

#include "drawable.hpp"
#include "sdf.hpp"

#include <algorithm>

namespace xel::graphics
{
// ── 圆角矩形 / 圆角环 / 内阴影 ──────────────────────────────────────
//
// 实现只有一种：解析 SDF（sdf.hpp）。每个图元提交 1 个 quad，
// fragment 里算到边界的有符号距离，用 fwidth 把边缘收成 1 物理像素。
// 顶点数与半径、与 DPI 都无关。
//
// 历史实现（几何细分、顶点级 fringe）已删除；需要对照请看 tag
// aa-fringe-imgui（顶点 fringe 版）与 7beb1c9 之前的几何细分。

// 圆角矩形填充。
inline void append_rounded_rect(DrawList& list, glm::vec4 rect, float radius, glm::vec4 color)
{
    append_sdf_rect(list, rect, radius, color);
}

// 圆角环（边框 / 焦点环）：外框 rect，厚度 thickness 向内收。
inline void append_rounded_ring(DrawList& list, glm::vec4 rect, float radius, float thickness,
                                glm::vec4 color)
{
    append_sdf_ring(list, rect, radius, thickness, color);
}

// 内阴影：从矩形上缘向内的多层半透明圆角条，重叠产生向下衰减的渐变。
// 每层都是独立的 SDF quad，AA 由 fragment 负责，不会像顶点 fringe 那样叠出亮边。
inline void append_inset_shadow(DrawList& list, glm::vec4 rect, float radius, float size,
                                glm::vec4 color)
{
    if (size <= 0.0f || color.a <= 0.0f || rect.z <= 0.0f || rect.w <= 0.0f) return;

    constexpr int layers = 4;
    for (int i = 0; i < layers; ++i) {
        const float k = static_cast<float>(i + 1) / static_cast<float>(layers);
        const float height = std::min(rect.w, size * k);
        append_sdf_rect(list, {rect.x, rect.y, rect.z, height}, radius, color);
    }
}
} // namespace xel::graphics
