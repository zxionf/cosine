#include "canvas.hpp"

#include "graphics/font_atlas.hpp"

#include <algorithm>
#include <cmath>

namespace gdemo
{
namespace
{
// 管线是预乘 alpha（One / OneMinusSrcAlpha）：半透明颜色必须先把 rgb 乘 alpha。
glm::vec4 premultiply(glm::vec4 color)
{
    return {color.r * color.a, color.g * color.a, color.b * color.a, color.a};
}

constexpr float kMinScreenWidth = 0.75f;
constexpr float kMaxScreenWidth = 256.0f;
} // namespace

// ── 帧开始 ────────────────────────────────────────────────────────
void Canvas::begin(graphics::FontAtlas* atlas, const Camera& camera, glm::vec2 viewport)
{
    atlas_ = atlas;
    camera_ = camera;
    viewport_ = viewport;
    list_.clear();
    list_.set_full_clip({0.0f, 0.0f, viewport.x, viewport.y});
}

glm::vec2 Canvas::white_uv() const
{
    return atlas_ != nullptr ? atlas_->white_uv() : glm::vec2{0.0f, 0.0f};
}

float Canvas::screen_length(float world_length) const
{
    return std::clamp(camera_.length_to_screen(world_length), kMinScreenWidth, kMaxScreenWidth);
}

// ── 顶点写入 ──────────────────────────────────────────────────────
void Canvas::quad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color)
{
    const glm::vec4 rgb = premultiply(color);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    auto& batch = list_.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(a, rgb, uv);
    batch.add_vertex(b, rgb, uv);
    batch.add_vertex(c, rgb, uv);
    batch.add_vertex(d, rgb, uv);
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}

void Canvas::quad_gradient(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d,
                           glm::vec4 ca, glm::vec4 cb)
{
    const glm::vec4 top = premultiply(ca);
    const glm::vec4 bottom = premultiply(cb);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    auto& batch = list_.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(a, top, uv);
    batch.add_vertex(b, top, uv);
    batch.add_vertex(c, bottom, uv);
    batch.add_vertex(d, bottom, uv);
    batch.indices.insert(batch.indices.end(), {
        base, base + 1, base + 2,
        base + 2, base + 3, base
    });
}

void Canvas::fan(glm::vec2 center, float radius, int segments, glm::vec4 color)
{
    if (radius <= 0.0f || segments < 3) return;
    const glm::vec4 rgb = premultiply(color);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    auto& batch = list_.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(center, rgb, uv);
    constexpr float kTau = 6.28318530718f;
    for (int i = 0; i <= segments; ++i) {
        const float angle = kTau * static_cast<float>(i) / static_cast<float>(segments);
        batch.add_vertex({center.x + radius * std::cos(angle),
                          center.y + radius * std::sin(angle)},
                         rgb, uv);
    }
    for (int i = 0; i < segments; ++i) {
        batch.indices.insert(batch.indices.end(), {
            base, base + 1 + static_cast<uint32_t>(i), base + 2 + static_cast<uint32_t>(i)
        });
    }
}

void Canvas::band(glm::vec2 center, float outer, float inner, int segments, glm::vec4 color)
{
    if (outer <= 0.0f || inner >= outer || segments < 3) return;
    const glm::vec4 rgb = premultiply(color);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    auto& batch = list_.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    constexpr float kTau = 6.28318530718f;
    for (int i = 0; i <= segments; ++i) {
        const float angle = kTau * static_cast<float>(i) / static_cast<float>(segments);
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        batch.add_vertex({center.x + outer * c, center.y + outer * s}, rgb, uv);
        batch.add_vertex({center.x + inner * c, center.y + inner * s}, rgb, uv);
    }
    for (int i = 0; i < segments; ++i) {
        const auto o0 = base + static_cast<uint32_t>(i) * 2;
        batch.indices.insert(batch.indices.end(), {
            o0, o0 + 1, o0 + 3,
            o0, o0 + 3, o0 + 2
        });
    }
}

void Canvas::strip(glm::vec2 a, glm::vec2 b, float screen_width, glm::vec4 color)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) return;
    const float half = std::max(screen_width, kMinScreenWidth) * 0.5f;
    const float nx = -dy / len * half;
    const float ny = dx / len * half;
    quad({a.x + nx, a.y + ny}, {a.x - nx, a.y - ny},
         {b.x - nx, b.y - ny}, {b.x + nx, b.y + ny}, color);
}

// ── 世界空间图元 ──────────────────────────────────────────────────
void Canvas::rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color)
{
    if (size.x <= 0.0f || size.y <= 0.0f) return;
    const glm::vec2 a = screen_of(pos);
    const glm::vec2 b = screen_of(pos + glm::vec2{size.x, 0.0f});
    const glm::vec2 c = screen_of(pos + size);
    const glm::vec2 d = screen_of(pos + glm::vec2{0.0f, size.y});
    quad(a, b, c, d, color);
}

void Canvas::gradient_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 top, glm::vec4 bottom)
{
    if (size.x <= 0.0f || size.y <= 0.0f) return;
    const glm::vec2 a = screen_of(pos);
    const glm::vec2 b = screen_of(pos + glm::vec2{size.x, 0.0f});
    const glm::vec2 c = screen_of(pos + size);
    const glm::vec2 d = screen_of(pos + glm::vec2{0.0f, size.y});
    quad_gradient(a, b, c, d, top, bottom);
}

void Canvas::rounded_rect(glm::vec2 pos, glm::vec2 size, float radius, glm::vec4 color)
{
    if (size.x <= 0.0f || size.y <= 0.0f) return;
    const glm::vec2 a = screen_of(pos);
    const float w = camera_.length_to_screen(size.x);
    const float h = camera_.length_to_screen(size.y);
    const float r = std::clamp(camera_.length_to_screen(radius), 0.0f,
                              std::min(w, h) * 0.5f);
    if (r < 0.5f) {
        quad(a, {a.x + w, a.y}, {a.x + w, a.y + h}, {a.x, a.y + h}, color);
        return;
    }
    // 中间十字 + 四条边 + 四个角扇。
    quad({a.x + r, a.y}, {a.x + w - r, a.y}, {a.x + w - r, a.y + h}, {a.x + r, a.y + h}, color);
    quad({a.x, a.y + r}, {a.x + r, a.y + r}, {a.x + r, a.y + h - r}, {a.x, a.y + h - r}, color);
    quad({a.x + w - r, a.y + r}, {a.x + w, a.y + r}, {a.x + w, a.y + h - r},
         {a.x + w - r, a.y + h - r}, color);

    const int segments = std::clamp(static_cast<int>(std::ceil(r * 0.5f)), 3, 16);
    constexpr float kHalfPi = 1.57079632679f;
    struct Corner { glm::vec2 center; float start; };
    const Corner corners[4] = {
        {{a.x + r, a.y + r}, 3.14159265f},
        {{a.x + w - r, a.y + r}, 4.71238898f},
        {{a.x + w - r, a.y + h - r}, 0.0f},
        {{a.x + r, a.y + h - r}, kHalfPi},
    };
    const glm::vec4 rgb = premultiply(color);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    for (const auto& corner : corners) {
        auto& batch = list_.batch();
        const auto base = static_cast<uint32_t>(batch.vertices.size());
        batch.add_vertex(corner.center, rgb, uv);
        for (int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float angle = corner.start + kHalfPi * t;
            batch.add_vertex({corner.center.x + r * std::cos(angle),
                              corner.center.y + r * std::sin(angle)}, rgb, uv);
        }
        for (int i = 0; i < segments; ++i) {
            batch.indices.insert(batch.indices.end(), {
                base, base + 1 + static_cast<uint32_t>(i), base + 2 + static_cast<uint32_t>(i)
            });
        }
    }
}

void Canvas::outline(glm::vec2 pos, glm::vec2 size, float width, glm::vec4 color)
{
    const float w = std::max(screen_length(width), 0.75f);
    const glm::vec2 a = screen_of(pos);
    const glm::vec2 b = screen_of(pos + size);
    if (b.x <= a.x || b.y <= a.y) return;
    quad(a, {b.x, a.y}, {b.x, a.y + w}, {a.x, a.y + w}, color);
    quad({a.x, b.y - w}, {b.x, b.y - w}, b, {a.x, b.y}, color);
    quad(a, {a.x + w, a.y}, {a.x + w, b.y}, {a.x, b.y}, color);
    quad({b.x - w, a.y}, {b.x, a.y}, {b.x, b.y}, {b.x - w, b.y}, color);
}

void Canvas::line(glm::vec2 a, glm::vec2 b, float width, glm::vec4 color)
{
    strip(screen_of(a), screen_of(b), screen_length(width), color);
}

void Canvas::dashed_line(glm::vec2 a, glm::vec2 b, float width, float dash, float gap,
                         glm::vec4 color)
{
    const glm::vec2 sa = screen_of(a);
    const glm::vec2 sb = screen_of(b);
    const glm::vec2 delta = sb - sa;
    const float total = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (total < 1e-6f) return;
    const glm::vec2 dir = delta / total;
    // dash / gap 是世界长度：换算到屏幕，至少 1px 以免缩小时退化。
    const float dash_px = std::max(camera_.length_to_screen(dash), 1.0f);
    const float gap_px = std::max(camera_.length_to_screen(gap), 1.0f);
    const float step = dash_px + gap_px;
    for (float t = 0.0f; t < total; t += step) {
        const float end = std::min(t + dash_px, total);
        strip(sa + dir * t, sa + dir * end, screen_length(width), color);
    }
}

void Canvas::triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color)
{
    const glm::vec4 rgb = premultiply(color);
    const glm::vec2 uv = white_uv();
    list_.set_texture(graphics::TextureSlot::Texture);
    auto& batch = list_.batch();
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    batch.add_vertex(screen_of(a), rgb, uv);
    batch.add_vertex(screen_of(b), rgb, uv);
    batch.add_vertex(screen_of(c), rgb, uv);
    batch.indices.insert(batch.indices.end(), {base, base + 1, base + 2});
}

void Canvas::circle(glm::vec2 center, float radius, glm::vec4 color)
{
    const float screen_radius = camera_.length_to_screen(radius);
    if (screen_radius < 0.25f) return;
    // 屏幕半径越大，弧越圆；缩到很小时退化为多边形（正是要观察的现象）。
    const int segments = std::clamp(static_cast<int>(std::ceil(screen_radius * 0.35f)), 12, 180);
    fan(screen_of(center), screen_radius, segments, color);
}

void Canvas::ring(glm::vec2 center, float radius, float thickness, glm::vec4 color)
{
    const float outer = camera_.length_to_screen(radius);
    const float inner = std::max(0.0f, outer - camera_.length_to_screen(thickness));
    if (outer < 0.25f || inner >= outer) return;
    const int segments = std::clamp(static_cast<int>(std::ceil(outer * 0.35f)), 12, 180);
    band(screen_of(center), outer, inner, segments, color);
}

void Canvas::polyline(std::span<const glm::vec2> points, float width, glm::vec4 color)
{
    if (points.size() < 2) return;
    const float w = screen_length(width);
    for (std::size_t i = 1; i < points.size(); ++i) {
        strip(screen_of(points[i - 1]), screen_of(points[i]), w, color);
    }
}

void Canvas::grid(glm::vec2 min, glm::vec2 max, float step, glm::vec4 color)
{
    if (step <= 0.0f) return;
    // 网格在屏幕上太密就不画（缩放实验里能明显看到「细节按缩放启用/消失」）。
    if (camera_.length_to_screen(step) < 3.0f) return;
    for (float x = min.x; x <= max.x + 0.001f; x += step) {
        line({x, min.y}, {x, max.y}, 1.0f, color);
    }
    for (float y = min.y; y <= max.y + 0.001f; y += step) {
        line({min.x, y}, {max.x, y}, 1.0f, color);
    }
}

void Canvas::text(glm::vec2 pos, float size, std::string_view text_value, glm::vec4 color)
{
    if (atlas_ == nullptr || text_value.empty()) return;
    const float screen_size = std::clamp(camera_.length_to_screen(size), 4.0f, 480.0f);
    const glm::vec2 at = screen_of(pos);
    // 与图元一致：文字也走预乘 alpha。
    atlas_->append_text(list_, at.x, at.y, screen_size, text_value, premultiply(color));
}

float Canvas::text_width(std::string_view text_value, float size) const
{
    if (text_value.empty()) return 0.0f;
    if (atlas_ == nullptr) return graphics::FontAtlas::estimate_text_width(text_value, size);
    return atlas_->measure_width(text_value, size);
}

// ── 裁剪 ──────────────────────────────────────────────────────────
void Canvas::push_clip(glm::vec2 pos, glm::vec2 size)
{
    const glm::vec4 bounds = camera_.screen_bounds(pos, size);
    list_.push_clip(bounds);
}

void Canvas::pop_clip()
{
    list_.pop_clip();
}

// ── 屏幕空间 ──────────────────────────────────────────────────────
void Canvas::screen_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color)
{
    if (size.x <= 0.0f || size.y <= 0.0f) return;
    quad(pos, {pos.x + size.x, pos.y}, pos + size, {pos.x, pos.y + size.y}, color);
}

void Canvas::screen_text(glm::vec2 pos, float size, std::string_view text_value, glm::vec4 color)
{
    if (atlas_ == nullptr || text_value.empty()) return;
    atlas_->append_text(list_, pos.x, pos.y, size, text_value, premultiply(color));
}

// ── 统计 / 输出 ───────────────────────────────────────────────────
std::size_t Canvas::vertex_count() const
{
    std::size_t total = 0;
    for (const auto& batch : list_.batches()) total += batch.vertices.size();
    return total;
}

std::size_t Canvas::index_count() const
{
    std::size_t total = 0;
    for (const auto& batch : list_.batches()) total += batch.indices.size();
    return total;
}

void Canvas::append_to(graphics::DrawList& out) const
{
    const glm::vec4 previous_clip = out.current_clip();
    for (const auto& src : list_.batches()) {
        if (src.vertices.empty() || src.indices.empty()) continue;
        out.set_clip(src.scissor);
        out.set_texture_slot(src.texture_slot);
        auto& dst = out.batch();
        const auto base = static_cast<uint32_t>(dst.vertices.size());
        dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
        dst.indices.reserve(dst.indices.size() + src.indices.size());
        for (const uint32_t index : src.indices) dst.indices.push_back(base + index);
    }
    out.set_clip(previous_clip);
}
} // namespace gdemo
