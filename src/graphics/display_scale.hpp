#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace xel::graphics
{
// ── 显示比例：UI 逻辑像素 ↔ framebuffer 物理像素的唯一事实 ────────────
//
// 两个互相正交的比例，之前散落在 Window / UiRuntime / Renderer / Application 四处，
// 这里合成一个值对象，谁需要换算就持有它的一份副本：
//
//   dpr        framebuffer / window（系统缩放、跨屏移动时变化）
//   ui_scale   用户比例（COSINE_UI_SCALE），逻辑像素 = 物理像素 / ui_scale
//
// 换算关系：
//   逻辑尺寸  = framebuffer 尺寸 / ui_scale
//   逻辑坐标  = 窗口坐标 × dpr / ui_scale        （输入）
//   物理像素  = 逻辑坐标 × ui_scale              （投影 / scissor）
struct DisplayScale
{
    static constexpr float kMinUiScale = 0.25f;
    static constexpr float kMaxUiScale = 4.0f;

    float dpr = 1.0f;      // framebuffer / window
    float ui_scale = 1.0f; // 逻辑像素相对于物理像素的缩小倍数

    static float clamp_ui(float scale)
    {
        if (!(scale > 0.0f) || !std::isfinite(scale)) return 1.0f;
        return std::clamp(scale, kMinUiScale, kMaxUiScale);
    }

    static DisplayScale make(float dpr_value, float ui_value)
    {
        DisplayScale result;
        result.dpr = (dpr_value > 0.0f && std::isfinite(dpr_value)) ? dpr_value : 1.0f;
        result.ui_scale = clamp_ui(ui_value);
        return result;
    }

    DisplayScale with_ui_scale(float ui_value) const { return make(dpr, ui_value); }
    DisplayScale with_dpr(float dpr_value) const { return make(dpr_value, ui_scale); }

    // 窗口坐标 → UI 逻辑坐标（输入事件用）。
    glm::vec2 window_to_logical(glm::vec2 window_pos) const
    {
        return window_pos * (dpr / ui_scale);
    }

    // framebuffer 物理像素 → UI 逻辑尺寸（root 尺寸用）。
    glm::vec2 framebuffer_to_logical(glm::vec2 framebuffer_size) const
    {
        return framebuffer_size / ui_scale;
    }

    bool operator==(const DisplayScale& other) const
    {
        return dpr == other.dpr && ui_scale == other.ui_scale;
    }
    bool operator!=(const DisplayScale& other) const { return !(*this == other); }
};
} // namespace xel::graphics
