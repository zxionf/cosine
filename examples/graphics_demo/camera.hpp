#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace gdemo
{
// ── 二维相机：世界坐标 ↔ 逻辑屏幕坐标 ──────────────────────────────
//
//   screen = world * zoom + pan
//
// 屏幕坐标是 UI 逻辑像素（左上原点、y 向下），与渲染投影一致；
// 因此相机只做缩放/平移，不做旋转与 y 翻转。
struct Camera
{
    static constexpr float kMinZoom = 0.05f;
    static constexpr float kMaxZoom = 64.0f;

    glm::vec2 pan{0.0f, 0.0f};
    float zoom = 1.0f;

    glm::vec2 to_screen(glm::vec2 world) const { return world * zoom + pan; }
    glm::vec2 to_world(glm::vec2 screen) const { return (screen - pan) / zoom; }
    float length_to_screen(float world_length) const { return world_length * zoom; }
    float length_to_world(float screen_length) const { return screen_length / zoom; }

    void pan_by(glm::vec2 delta) { pan += delta; }

    // 以屏幕点 anchor 为不动点缩放：该点下方的世界坐标保持不动。
    void zoom_at(glm::vec2 anchor, float factor)
    {
        if (!(factor > 0.0f) || !std::isfinite(factor)) return;
        const glm::vec2 world = to_world(anchor);
        zoom = std::clamp(zoom * factor, kMinZoom, kMaxZoom);
        pan = anchor - world * zoom;
    }

    // 让以原点为中心、尺寸 content 的世界矩形适配视口（含边距）。
    void fit(glm::vec2 viewport, glm::vec2 content, float margin = 48.0f)
    {
        const float usable_w = std::max(1.0f, viewport.x - margin * 2.0f);
        const float usable_h = std::max(1.0f, viewport.y - margin * 2.0f);
        const float fit_zoom = std::min(usable_w / std::max(1.0f, content.x),
                                        usable_h / std::max(1.0f, content.y));
        zoom = std::clamp(fit_zoom, kMinZoom, kMaxZoom);
        // 世界原点落在视口中心。
        pan = viewport * 0.5f;
    }

    // 世界矩形在屏幕上的包围盒（供 HUD / 拾取用）。
    glm::vec4 screen_bounds(glm::vec2 pos, glm::vec2 size) const
    {
        const glm::vec2 p0 = to_screen(pos);
        const glm::vec2 p1 = to_screen(pos + size);
        return {p0.x, p0.y, p1.x - p0.x, p1.y - p0.y};
    }

    bool operator==(const Camera& other) const { return pan == other.pan && zoom == other.zoom; }
    bool operator!=(const Camera& other) const { return !(*this == other); }
};
} // namespace gdemo
