#pragma once

#include "drawable.hpp"
#include "font_atlas.hpp"

#include <algorithm>
#include <cmath>

namespace xel::graphics
{
// 圆角矩形 / 圆角环 / 内阴影的几何构造。
// 不做 SDF 与抗锯齿：与现有矩形、线段一样是硬边，靠几何细分逼近圆弧。
namespace box_detail
{
constexpr float kPi = 3.14159265358979323846f;

inline glm::vec2 white_uv()
{
    auto* atlas = active_font_atlas();
    return atlas ? atlas->white_uv() : glm::vec2{0.0f, 0.0f};
}

// 管线使用预乘 alpha 混合（One / OneMinusSrcAlpha）：半透明颜色必须先把 rgb 乘以 alpha。
inline glm::vec4 premultiply(glm::vec4 color)
{
    return {color.r * color.a, color.g * color.a, color.b * color.a, color.a};
}

// 角的分段数：半径越大弧越长，段数按弧长自适应，上限 6 段。
inline int corner_segments(float radius)
{
    return std::clamp(static_cast<int>(std::ceil(radius * 0.6f)), 2, 6);
}

inline void push_quad(DrawList& list, glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color)
{
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(a, color, uv);
    batch.add_vertex(b, color, uv);
    batch.add_vertex(c, color, uv);
    batch.add_vertex(d, color, uv);
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}

inline void push_rect(DrawList& list, float x, float y, float w, float h, glm::vec4 color)
{
    if (w <= 0.0f || h <= 0.0f) return;
    push_quad(list,
              {x, y},
              {x + w, y},
              {x + w, y + h},
              {x, y + h},
              color);
}

// 四分之一圆盘（角扇）：中心点 + 弧上 seg+1 个点。
inline void push_corner_fan(DrawList& list, glm::vec2 center, float radius,
                            float start_angle, float end_angle, int seg, glm::vec4 color)
{
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(center, color, uv);
    for (int i = 0; i <= seg; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(seg);
        const float angle = start_angle + (end_angle - start_angle) * t;
        const glm::vec2 p{center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)};
        batch.add_vertex(p, color, uv);
    }
    for (int i = 0; i < seg; ++i) {
        batch.indices.insert(batch.indices.end(), {
            base, base + 1 + static_cast<uint32_t>(i), base + 2 + static_cast<uint32_t>(i)
        });
    }
}

// 角环四边形：外弧与内弧同角度的一对点连成条带。
inline void push_corner_band(DrawList& list, glm::vec2 center, float outer_radius, float inner_radius,
                             float start_angle, float end_angle, int seg, glm::vec4 color)
{
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
    for (int i = 0; i <= seg; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(seg);
        const float angle = start_angle + (end_angle - start_angle) * t;
        const float cos_a = std::cos(angle);
        const float sin_a = std::sin(angle);
        batch.add_vertex({center.x + outer_radius * cos_a, center.y + outer_radius * sin_a}, color, uv);
        batch.add_vertex({center.x + inner_radius * cos_a, center.y + inner_radius * sin_a}, color, uv);
    }
    for (int i = 0; i < seg; ++i) {
        const uint32_t o0 = base + static_cast<uint32_t>(i) * 2;
        const uint32_t i0 = o0 + 1;
        const uint32_t o1 = o0 + 2;
        const uint32_t i1 = o0 + 3;
        batch.indices.insert(batch.indices.end(), {o0, o1, i1, i1, i0, o0});
    }
}
} // namespace box_detail

// 圆角矩形：九宫格不重叠切分（中心 + 四边矩形 + 四角扇形）。
// 区域互不重叠，预乘 alpha 下半透明不会二次提亮，因此半透明圆角面板可直接使用。
inline void append_rounded_rect(DrawList& list, glm::vec4 rect, float radius, glm::vec4 color)
{
    const float w = rect.z;
    const float h = rect.w;
    if (w <= 0.0f || h <= 0.0f || color.a <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const glm::vec4 premul = box_detail::premultiply(color);
    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    if (r < 0.75f) {
        box_detail::push_rect(list, rect.x, rect.y, w, h, premul);
        return;
    }

    const float x = rect.x;
    const float y = rect.y;
    const float mid_w = w - r * 2.0f;
    const float mid_h = h - r * 2.0f;
    box_detail::push_rect(list, x + r, y, mid_w, r, premul);
    box_detail::push_rect(list, x + r, y + h - r, mid_w, r, premul);
    box_detail::push_rect(list, x, y + r, r, mid_h, premul);
    box_detail::push_rect(list, x + w - r, y + r, r, mid_h, premul);
    box_detail::push_rect(list, x + r, y + r, mid_w, mid_h, premul);

    const int seg = box_detail::corner_segments(r);
    constexpr float half_pi = box_detail::kPi * 0.5f;
    constexpr float pi = box_detail::kPi;
    // 坐标系 y 向下：左上角覆盖 180°..270°，右上 270°..360°，右下 0°..90°，左下 90°..180°。
    box_detail::push_corner_fan(list, {x + r, y + r}, r, pi, pi + half_pi, seg, premul);
    box_detail::push_corner_fan(list, {x + w - r, y + r}, r, pi + half_pi, pi * 2.0f, seg, premul);
    box_detail::push_corner_fan(list, {x + w - r, y + h - r}, r, 0.0f, half_pi, seg, premul);
    box_detail::push_corner_fan(list, {x + r, y + h - r}, r, half_pi, pi, seg, premul);
}

// 圆角环：外圆角矩形与内圆角矩形之间的条带，内部不填充、不遮挡内容。
// 边框与焦点环共用；radius 退化为 0 时就是四条直边矩形。
inline void append_rounded_ring(DrawList& list, glm::vec4 rect, float radius, float thickness, glm::vec4 color)
{
    const float w = rect.z;
    const float h = rect.w;
    if (w <= 0.0f || h <= 0.0f || color.a <= 0.0f) return;
    const float t = std::clamp(thickness, 0.0f, std::min(w, h) * 0.5f);
    if (t <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const glm::vec4 premul = box_detail::premultiply(color);
    const float x = rect.x;
    const float y = rect.y;

    // 直边部分：上下两条 + 左右两条，避让四角区域，彼此不重叠。
    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    box_detail::push_rect(list, x + r, y, w - r * 2.0f, t, premul);
    box_detail::push_rect(list, x + r, y + h - t, w - r * 2.0f, t, premul);
    box_detail::push_rect(list, x, y + r, t, h - r * 2.0f, premul);
    box_detail::push_rect(list, x + w - t, y + r, t, h - r * 2.0f, premul);
    if (r < 0.75f) return;

    const float inner_radius = r - t;
    const int seg = box_detail::corner_segments(r);
    constexpr float half_pi = box_detail::kPi * 0.5f;
    constexpr float pi = box_detail::kPi;

    const glm::vec2 centers[4] = {
        {x + r, y + r},
        {x + w - r, y + r},
        {x + w - r, y + h - r},
        {x + r, y + h - r},
    };
    const float starts[4] = {pi, pi + half_pi, 0.0f, half_pi};

    for (int i = 0; i < 4; ++i) {
        const float start = starts[i];
        const float end = start + half_pi;
        if (inner_radius <= 0.25f) {
            box_detail::push_corner_fan(list, centers[i], r, start, end, seg, premul);
        } else {
            box_detail::push_corner_band(list, centers[i], r, inner_radius, start, end, seg, premul);
        }
    }
}

// 内阴影：从矩形上缘向内的多层半透明圆角条，重叠产生向下衰减的渐变。
inline void append_inset_shadow(DrawList& list, glm::vec4 rect, float radius, float size, glm::vec4 color)
{
    if (size <= 0.0f || color.a <= 0.0f || rect.z <= 0.0f || rect.w <= 0.0f) return;

    constexpr int layers = 4;
    for (int i = 0; i < layers; ++i) {
        const float k = static_cast<float>(i + 1) / static_cast<float>(layers);
        const float height = std::min(rect.w, size * k);
        append_rounded_rect(list, {rect.x, rect.y, rect.z, height}, radius, color);
    }
}
} // namespace xel::graphics
