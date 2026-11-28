#pragma once

#include "drawable.hpp"
#include "font_atlas.hpp"
#include "sdf.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace xel::graphics
{
// ── 圆角矩形 / 圆角环 / 内阴影 ──────────────────────────────────────
//
// 三代实现并存，**调用方只用前两个名字**，历史实现保留做对照与回退：
//
//   append_rounded_rect / append_rounded_ring        ← 当前实现
//       解析 SDF（sdf.hpp）：只提交 1 个 quad，fragment 里算距离，
//       用 fwidth 把边缘收成 1 物理像素。顶点数与半径/DPI 无关。
//
//   append_rounded_rect_fringe / append_rounded_ring_fringe
//       过渡方案：整条轮廓 + 1px alpha 边带（Dear ImGui 的 fringe 做法）。
//       已被 SDF 取代，保留用于 A/B 对照与回退。
//
//   append_rounded_rect_hard / append_rounded_ring_hard
//       最初的几何细分：中心 + 四边矩形 + 四角扇形，硬边。
//
// 三者都保持「预乘 alpha」约定：fragment/顶点给的覆盖率同时乘 rgb 与 a。
namespace box_detail
{
constexpr float kPi = 3.14159265358979323846f;

// 弧的弦高误差上限（物理像素）与段数上限。
// ImGui 的 ImGuiStyle::CircleTessellationMaxError 默认 0.3，这里取同量级。
constexpr float kMaxArcErrorPx = 0.3f;
constexpr int kMaxArcSegments = 32;
constexpr int kMaxPathPoints = 4 * kMaxArcSegments;
constexpr float kMinRoundedRadius = 0.75f;

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

// fringe 外圈：颜色必须全 0（预乘坐标下 alpha=0 即透明，且不能带 rgb）。
inline glm::vec4 transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

// 1 物理像素换算到逻辑像素。pixel_ratio 缺失时按 1 处理（等价于旧行为）。
inline float fringe_width(const DrawList& list) { return 1.0f / list.pixel_ratio(); }

// 弧的段数：按屏幕半径与允许的弦高误差动态定。
//   segments = ceil((π/2) / acos(1 - error / r_px))
// 半径 8px、误差 0.3px 时得到 6 段；半径 100px 时得到 21 段。
inline int arc_segments(float radius_px)
{
    const float r = std::max(radius_px, 0.0f);
    if (r <= kMaxArcErrorPx) return 2;
    const float ratio = std::clamp(1.0f - kMaxArcErrorPx / r, -1.0f, 1.0f);
    const float step = std::acos(ratio);
    if (!(step > 1e-4f)) return kMaxArcSegments;
    const int segments = static_cast<int>(std::ceil((kPi * 0.5f) / step));
    return std::clamp(segments, 2, kMaxArcSegments);
}

// 一条闭合轮廓 + 每个点的外扩偏移（fringe）。
struct Path
{
    std::array<glm::vec2, kMaxPathPoints> points;
    std::array<glm::vec2, kMaxPathPoints> offsets;
    int count = 0;

    bool valid() const { return count >= 4; }
};

// 追加一段圆弧：含起点、不含终点，所以相邻两段之间自然连出一条直边。
inline void add_arc(Path& path, glm::vec2 center, float radius, float start, float end, int segments)
{
    for (int i = 0; i < segments; ++i) {
        if (path.count >= kMaxPathPoints) return;
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = start + (end - start) * t;
        path.points[path.count++] = {
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle),
        };
    }
}

// 每点的外法线 = 相邻两边法线的平均，再按夹角修正长度，
// 保证沿法线方向投影出去仍然是 fringe 像素（ImGui 同款处理）。
inline void compute_offsets(Path& path, float fringe)
{
    if (!path.valid()) return;
    const int count = path.count;
    for (int i = 0; i < count; ++i) {
        const glm::vec2& current = path.points[i];
        const glm::vec2& previous = path.points[(i + count - 1) % count];
        const glm::vec2& next = path.points[(i + 1) % count];

        const glm::vec2 e0 = current - previous;
        const glm::vec2 e1 = next - current;
        const float l0 = std::sqrt(e0.x * e0.x + e0.y * e0.y);
        const float l1 = std::sqrt(e1.x * e1.x + e1.y * e1.y);
        // y 向下、轮廓顺时针：外法线 = (dy, -dx)。
        const glm::vec2 n0 = l0 > 1e-6f ? glm::vec2{e0.y / l0, -e0.x / l0} : glm::vec2{0.0f};
        const glm::vec2 n1 = l1 > 1e-6f ? glm::vec2{e1.y / l1, -e1.x / l1} : glm::vec2{0.0f};

        glm::vec2 normal = n0 + n1;
        const float len = std::sqrt(normal.x * normal.x + normal.y * normal.y);
        if (len > 1e-5f) {
            normal /= len;
        } else {
            normal = n1; // 180° 折返：退化为当前边的法线。
        }
        const float cos_half = std::max(0.35f, normal.x * n1.x + normal.y * n1.y);
        path.offsets[i] = normal * (fringe / cos_half);
    }
}

// 圆角矩形轮廓（顺时针，从左上角弧起）。
inline Path build_rect_path(glm::vec4 rect, float radius, int segments, float fringe)
{
    Path path;
    const float x = rect.x;
    const float y = rect.y;
    const float w = rect.z;
    const float h = rect.w;
    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    constexpr float half_pi = kPi * 0.5f;

    // 左上(π..1.5π) → 右上(1.5π..2π) → 右下(0..0.5π) → 左下(0.5π..π)
    add_arc(path, {x + r, y + r}, r, kPi, kPi + half_pi, segments);
    add_arc(path, {x + w - r, y + r}, r, kPi + half_pi, kPi * 2.0f, segments);
    add_arc(path, {x + w - r, y + h - r}, r, 0.0f, half_pi, segments);
    add_arc(path, {x + r, y + h - r}, r, half_pi, kPi, segments);

    compute_offsets(path, fringe);
    return path;
}

// 环的内轮廓：同一组角度、同段数，整体内缩 inset（保证与外围点一一对应）。
inline Path build_inset_path(glm::vec4 rect, float radius, float inset, int segments, float fringe)
{
    const glm::vec4 inner{rect.x + inset, rect.y + inset, rect.z - inset * 2.0f,
                          rect.w - inset * 2.0f};
    return build_rect_path(inner, std::max(0.0f, radius - inset), segments, fringe);
}

inline void push_rect(DrawList& list, float x, float y, float w, float h, glm::vec4 color)
{
    if (w <= 0.0f || h <= 0.0f) return;
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex({x, y}, color, uv);
    batch.add_vertex({x + w, y}, color, uv);
    batch.add_vertex({x + w, y + h}, color, uv);
    batch.add_vertex({x, y + h}, color, uv);
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}

// 填充：中心 fan（凸多边形，不重叠）+ 可选的外沿 fringe 带。
// 顶点复用：轮廓点既是 fan 的边界，也是 fringe 的内圈，所以只写一份。
inline void append_fill(DrawList& list, const Path& path, glm::vec4 color, bool with_fringe)
{
    if (!path.valid()) return;
    const int count = path.count;
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const glm::vec4 inner = premultiply(color);
    const glm::vec4 outer = transparent();

    const auto base = static_cast<uint32_t>(batch.vertices.size());
    // 重心：轮廓的顶点平均值（凸多边形，一定在内部）。
    glm::vec2 center{0.0f, 0.0f};
    for (int i = 0; i < count; ++i) center += path.points[i];
    center /= static_cast<float>(count);
    batch.add_vertex(center, inner, uv);
    const uint32_t center_index = base;

    for (int i = 0; i < count; ++i) {
        batch.add_vertex(path.points[i], inner, uv);
    }
    const uint32_t in_first = base + 1;

    if (with_fringe) {
        for (int i = 0; i < count; ++i) {
            batch.add_vertex(path.points[i] + path.offsets[i], outer, uv);
        }
        const uint32_t out_first = in_first + static_cast<uint32_t>(count);
        for (int i = 0; i < count; ++i) {
            const uint32_t i0 = in_first + static_cast<uint32_t>(i);
            const uint32_t i1 = in_first + static_cast<uint32_t>((i + 1) % count);
            const uint32_t o0 = out_first + static_cast<uint32_t>(i);
            const uint32_t o1 = out_first + static_cast<uint32_t>((i + 1) % count);
            batch.indices.insert(batch.indices.end(), {i0, i1, o1, i0, o1, o0});
        }
    }

    for (int i = 0; i < count; ++i) {
        const uint32_t i0 = in_first + static_cast<uint32_t>(i);
        const uint32_t i1 = in_first + static_cast<uint32_t>((i + 1) % count);
        batch.indices.insert(batch.indices.end(), {center_index, i0, i1});
    }
}

// 环：外轮廓与内轮廓之间的条带 + 两侧各自的 1px 渐变带。
//   outer_fringe = false 时跳过外侧（填充已经铺到外沿，避免 1px 重叠提亮）。
inline void append_ring(DrawList& list, const Path& outer_path, const Path& inner_path,
                        glm::vec4 color, bool outer_fringe, bool inner_fringe)
{
    if (!outer_path.valid() || !inner_path.valid()) return;
    if (outer_path.count != inner_path.count) return;

    const int count = outer_path.count;
    auto& batch = list.batch();
    const glm::vec2 uv = white_uv();
    const glm::vec4 inner = premultiply(color);
    const glm::vec4 transparent_color = transparent();

    const auto base = static_cast<uint32_t>(batch.vertices.size());
    for (int i = 0; i < count; ++i) batch.add_vertex(outer_path.points[i], inner, uv);
    const uint32_t outer_first = base;
    for (int i = 0; i < count; ++i) batch.add_vertex(inner_path.points[i], inner, uv);
    const uint32_t inner_first = base + static_cast<uint32_t>(count);

    // 条带：外[i], 外[i+1], 内[i+1], 内[i]
    for (int i = 0; i < count; ++i) {
        const uint32_t a0 = outer_first + static_cast<uint32_t>(i);
        const uint32_t a1 = outer_first + static_cast<uint32_t>((i + 1) % count);
        const uint32_t b0 = inner_first + static_cast<uint32_t>(i);
        const uint32_t b1 = inner_first + static_cast<uint32_t>((i + 1) % count);
        batch.indices.insert(batch.indices.end(), {a0, a1, b1, a0, b1, b0});
    }

    if (outer_fringe) {
        const auto out_base = static_cast<uint32_t>(batch.vertices.size());
        for (int i = 0; i < count; ++i) {
            batch.add_vertex(outer_path.points[i] + outer_path.offsets[i], transparent_color, uv);
        }
        for (int i = 0; i < count; ++i) {
            const uint32_t a0 = outer_first + static_cast<uint32_t>(i);
            const uint32_t a1 = outer_first + static_cast<uint32_t>((i + 1) % count);
            const uint32_t o0 = out_base + static_cast<uint32_t>(i);
            const uint32_t o1 = out_base + static_cast<uint32_t>((i + 1) % count);
            batch.indices.insert(batch.indices.end(), {a0, a1, o1, a0, o1, o0});
        }
    }

    if (inner_fringe) {
        const auto out_base = static_cast<uint32_t>(batch.vertices.size());
        // 内轮廓的外法线朝外（朝条带），向内扩要取负。
        for (int i = 0; i < count; ++i) {
            batch.add_vertex(inner_path.points[i] - inner_path.offsets[i], transparent_color, uv);
        }
        for (int i = 0; i < count; ++i) {
            const uint32_t b0 = inner_first + static_cast<uint32_t>(i);
            const uint32_t b1 = inner_first + static_cast<uint32_t>((i + 1) % count);
            const uint32_t o0 = out_base + static_cast<uint32_t>(i);
            const uint32_t o1 = out_base + static_cast<uint32_t>((i + 1) % count);
            batch.indices.insert(batch.indices.end(), {b0, o0, o1, b0, o1, b1});
        }
    }
}

// ── 旧路径（硬边几何细分）用的工具，保留以支持对照 ──────────────────
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
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        batch.add_vertex({center.x + outer_radius * c, center.y + outer_radius * s}, color, uv);
        batch.add_vertex({center.x + inner_radius * c, center.y + inner_radius * s}, color, uv);
    }
    for (int i = 0; i < seg; ++i) {
        const auto o0 = base + static_cast<uint32_t>(i) * 2;
        batch.indices.insert(batch.indices.end(), {
            o0, o0 + 1, o0 + 3,
            o0, o0 + 3, o0 + 2
        });
    }
}
} // namespace box_detail

// ── 旧实现：几何细分，硬边（对照 / 回退用）────────────────────────
inline void append_rounded_rect_hard(DrawList& list, glm::vec4 rect, float radius, glm::vec4 color)
{
    const float w = rect.z;
    const float h = rect.w;
    if (w <= 0.0f || h <= 0.0f || color.a <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const glm::vec4 premul = box_detail::premultiply(color);
    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    if (r < box_detail::kMinRoundedRadius) {
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
    box_detail::push_corner_fan(list, {x + r, y + r}, r, pi, pi + half_pi, seg, premul);
    box_detail::push_corner_fan(list, {x + w - r, y + r}, r, pi + half_pi, pi * 2.0f, seg, premul);
    box_detail::push_corner_fan(list, {x + w - r, y + h - r}, r, 0.0f, half_pi, seg, premul);
    box_detail::push_corner_fan(list, {x + r, y + h - r}, r, half_pi, pi, seg, premul);
}

inline void append_rounded_ring_hard(DrawList& list, glm::vec4 rect, float radius, float thickness,
                                     glm::vec4 color)
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

    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    box_detail::push_rect(list, x + r, y, w - r * 2.0f, t, premul);
    box_detail::push_rect(list, x + r, y + h - t, w - r * 2.0f, t, premul);
    box_detail::push_rect(list, x, y + r, t, h - r * 2.0f, premul);
    box_detail::push_rect(list, x + w - t, y + r, t, h - r * 2.0f, premul);
    if (r < box_detail::kMinRoundedRadius) return;

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

// ── 过渡实现：整条轮廓 + 1px alpha 边带（顶点级抗锯齿）────────────
//
// 已被解析 SDF（sdf.hpp）取代，保留用于 A/B 对照与回退，P5 删除。
// with_fringe = false 时只画填充。
inline void append_rounded_rect_fringe(DrawList& list, glm::vec4 rect, float radius,
                                       glm::vec4 color, bool with_fringe = true)
{
    const float w = rect.z;
    const float h = rect.w;
    if (w <= 0.0f || h <= 0.0f || color.a <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    if (r < box_detail::kMinRoundedRadius) {
        // 小圆角退化为矩形：轴对齐、像素对齐时不需要边带。
        box_detail::push_rect(list, rect.x, rect.y, w, h, box_detail::premultiply(color));
        return;
    }

    const float fringe = box_detail::fringe_width(list);
    const int segments = box_detail::arc_segments(r * list.pixel_ratio());
    const box_detail::Path path = box_detail::build_rect_path(rect, r, segments, fringe);
    box_detail::append_fill(list, path, color, with_fringe);
}

inline void append_rounded_ring_fringe(DrawList& list, glm::vec4 rect, float radius,
                                       float thickness, glm::vec4 color)
{
    const float w = rect.z;
    const float h = rect.w;
    if (w <= 0.0f || h <= 0.0f || color.a <= 0.0f) return;
    const float t = std::clamp(thickness, 0.0f, std::min(w, h) * 0.5f);
    if (t <= 0.0f) return;
    list.set_texture(TextureSlot::Texture);

    const float r = std::clamp(radius, 0.0f, std::min(w, h) * 0.5f);
    if (r < box_detail::kMinRoundedRadius) {
        // 直角边框：四条直边矩形，互不重叠（与旧实现一致）。
        const glm::vec4 premul = box_detail::premultiply(color);
        box_detail::push_rect(list, rect.x, rect.y, w, t, premul);
        box_detail::push_rect(list, rect.x, rect.y + h - t, w, t, premul);
        box_detail::push_rect(list, rect.x, rect.y + t, t, h - t * 2.0f, premul);
        box_detail::push_rect(list, rect.x + w - t, rect.y + t, t, h - t * 2.0f, premul);
        return;
    }

    const float fringe = box_detail::fringe_width(list);
    const int segments = box_detail::arc_segments(r * list.pixel_ratio());
    const box_detail::Path outer = box_detail::build_rect_path(rect, r, segments, fringe);
    const box_detail::Path inner = box_detail::build_inset_path(rect, r, t, segments, fringe);

    // 填充已经把颜色铺到外沿时，外侧边带不再外扩，避免 1px 重叠。
    const bool outer_fringe = !list.skip_outer_fringe();
    box_detail::append_ring(list, outer, inner, color, outer_fringe, true);
}

// ── 现行实现：解析 SDF（一个 quad，fragment 里算距离）────────────
//
// 调用签名与旧几何路径完全一致（68 个调用点零改动）。
// 顶点只有 4 个，不再需要按 DPI 细分、也不再需要 fringe。
inline void append_rounded_rect(DrawList& list, glm::vec4 rect, float radius, glm::vec4 color)
{
    append_sdf_rect(list, rect, radius, color);
}

inline void append_rounded_ring(DrawList& list, glm::vec4 rect, float radius, float thickness,
                                glm::vec4 color)
{
    append_sdf_ring(list, rect, radius, thickness, color);
}

// ── 内阴影：从矩形上缘向内的多层半透明圆角条，重叠产生向下衰减的渐变。
// SDF 填充本身没有外扩边带，四层叠加只需直接画。
inline void append_inset_shadow(DrawList& list, glm::vec4 rect, float radius, float size, glm::vec4 color)
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
