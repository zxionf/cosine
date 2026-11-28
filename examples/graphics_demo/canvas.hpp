#pragma once

#include "camera.hpp"

#include "graphics/drawable.hpp"
#include "graphics/font_atlas.hpp"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

namespace gdemo
{
// 本示例只关心绘制：把 xel::graphics 缩短一个前缀。
namespace graphics = xel::graphics;

// ── 立即模式画布：世界坐标进，顶点进 DrawList ────────────────────────
//
// 只依赖 graphics（DrawList / Drawable / FontAtlas），不认识 UI，也不认识 Vulkan：
//   begin(atlas, camera, viewport) → rect/circle/line/text… → gfx().draw(canvas)
//
// 缩放是画布自己的事：世界坐标经 camera 变成屏幕坐标，线宽与文字字号一起乘 zoom，
// 圆的细分段数按屏幕半径自适应（放大后弧更圆，缩小后退化成多边形）。
//
// 它实现 graphics::Drawable，所以同一份内容既能进窗口帧，也能离屏 capture 成 PNG。
class Canvas final : public graphics::Drawable
{
public:
    void begin(graphics::FontAtlas* atlas, const Camera& camera, glm::vec2 viewport);

    const Camera& camera() const { return camera_; }
    glm::vec2 viewport() const { return viewport_; }

    // ── 世界空间图元 ──────────────────────────────────────────────
    void rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color);
    void gradient_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 top, glm::vec4 bottom);
    void rounded_rect(glm::vec2 pos, glm::vec2 size, float radius, glm::vec4 color);
    void outline(glm::vec2 pos, glm::vec2 size, float width, glm::vec4 color);
    void line(glm::vec2 a, glm::vec2 b, float width, glm::vec4 color);
    void dashed_line(glm::vec2 a, glm::vec2 b, float width, float dash, float gap, glm::vec4 color);
    void triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color);
    void circle(glm::vec2 center, float radius, glm::vec4 color);
    void ring(glm::vec2 center, float radius, float thickness, glm::vec4 color);
    void polyline(std::span<const glm::vec2> points, float width, glm::vec4 color);
    void grid(glm::vec2 min, glm::vec2 max, float step, glm::vec4 color);
    // 文字的 pos 是行框左上角，size 是世界字号（会乘 zoom 再栅格化）。
    void text(glm::vec2 pos, float size, std::string_view text, glm::vec4 color);
    float text_width(std::string_view text, float size) const;

    // 裁剪：世界矩形 → 屏幕 scissor；不同 clip 会切成不同 batch。
    void push_clip(glm::vec2 pos, glm::vec2 size);
    void pop_clip();

    // ── 屏幕空间（HUD 不随相机缩放）────────────────────────────────
    void screen_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color);
    void screen_text(glm::vec2 pos, float size, std::string_view text, glm::vec4 color);

    // ── 统计（本帧已提交的顶点）────────────────────────────────────
    const std::vector<graphics::DrawBatch>& batches() const { return list_.batches(); }
    std::size_t batch_count() const { return list_.batches().size(); }
    std::size_t vertex_count() const;
    std::size_t index_count() const;

    // ── graphics::Drawable ────────────────────────────────────────
    void append_to(graphics::DrawList& out) const override;
    graphics::TextureSlot texture_slot() const override { return graphics::TextureSlot::Texture; }

private:
    // 屏幕空间顶点写入（所有入口最终都走这里）。
    void quad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color);
    void quad_gradient(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d,
                       glm::vec4 ca, glm::vec4 cb);
    void fan(glm::vec2 center, float radius, int segments, glm::vec4 color);
    void band(glm::vec2 center, float outer, float inner, int segments, glm::vec4 color);
    void strip(glm::vec2 a, glm::vec2 b, float screen_width, glm::vec4 color);
    glm::vec2 white_uv() const;
    glm::vec2 screen_of(glm::vec2 world) const { return camera_.to_screen(world); }
    float screen_length(float world_length) const;

    graphics::DrawList list_;
    Camera camera_{};
    glm::vec2 viewport_{0.0f, 0.0f};
    graphics::FontAtlas* atlas_ = nullptr;
};
} // namespace gdemo
