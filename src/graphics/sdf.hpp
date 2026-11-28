#pragma once

#include "drawable.hpp"

#include <algorithm>

namespace xel::graphics
{
// ── 解析 SDF 图元（一个 quad，fragment 里算距离）────────────────────
//
// 提交约定（与 ui.slang 的 fragMain 一一对应）：
//   uv    局部坐标 = 顶点相对矩形中心的偏移（单位：逻辑像素）
//   half  未含圆角的外半尺寸
//   extra (radius, thickness)
//   kind  VertexKind::Rect / VertexKind::Ring
//
// 顶点只写 4 个，抗锯齿在 fragment 里按 fwidth 收成 1 物理像素，
// 因此不需要 fringe、不需要按 DPI 重新细分，也不需要纹理。
inline void append_sdf_rect(DrawList& list, glm::vec4 rect, float radius, glm::vec4 color)
{
    if (rect.z <= 0.0f || rect.w <= 0.0f || color.a <= 0.0f) return;
    list.set_texture(TextureSlot::Texture); // 不采样，但保持同槽以便与其它图元合批

    const glm::vec2 half{rect.z * 0.5f, rect.w * 0.5f};
    const glm::vec2 center{rect.x + half.x, rect.y + half.y};
    const float r = std::clamp(radius, 0.0f, std::min(half.x, half.y));

    auto& batch = list.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    const auto kind = static_cast<uint32_t>(VertexKind::Rect);
    const glm::vec2 corners[4] = {
        {-half.x, -half.y},
        { half.x, -half.y},
        { half.x,  half.y},
        {-half.x,  half.y},
    };
    for (const glm::vec2& corner : corners) {
        batch.add_vertex(center + corner, color, corner, half, {r, 0.0f}, kind);
    }
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}

// 圆角环：外框 rect + 外圆角 radius，厚度 thickness 向内收。
inline void append_sdf_ring(DrawList& list, glm::vec4 rect, float radius, float thickness,
                     glm::vec4 color)
{
    if (rect.z <= 0.0f || rect.w <= 0.0f || color.a <= 0.0f) return;
    const glm::vec2 half{rect.z * 0.5f, rect.w * 0.5f};
    const float t = std::clamp(thickness, 0.0f, std::min(half.x, half.y));
    if (t <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const glm::vec2 center{rect.x + half.x, rect.y + half.y};
    const float r = std::clamp(radius, 0.0f, std::min(half.x, half.y));

    auto& batch = list.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    const auto kind = static_cast<uint32_t>(VertexKind::Ring);
    const glm::vec2 corners[4] = {
        {-half.x, -half.y},
        { half.x, -half.y},
        { half.x,  half.y},
        {-half.x,  half.y},
    };
    for (const glm::vec2& corner : corners) {
        batch.add_vertex(center + corner, color, corner, half, {r, t}, kind);
    }
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}
} // namespace xel::graphics
