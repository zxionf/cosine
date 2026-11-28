#pragma once

// ── 渲染后端接口 ──────────────────────────────────────────────────
//
// 应用层只依赖这个接口，不依赖 Vulkan：
//   一帧：begin(clear) → draw(drawable) → end()   （窗口目标在 end() 内 present）
//   离屏：capture(drawable, w, h)                  （返回 RGBA8，行优先、顶部起）
//
// 消费方（examples / 快照测试）因此可以在不想碰窗口时只调 capture()；
// 将来接 NullBackend / 软件光栅也只需实现本接口。

#include "graphics/drawable.hpp"
#include "graphics/frame_stats.hpp"
#include "graphics/font_atlas.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace xel::backend
{
class RenderBackend
{
public:
    virtual ~RenderBackend() = default;

    virtual graphics::FontAtlas& font_atlas() = 0;
    virtual const graphics::FrameStats& stats() const = 0;

    // DPI：逻辑坐标 → framebuffer 像素，同时决定字体栅格化比例。
    virtual void set_scale(float scale) = 0;

    // 一帧。clear_color 通常取主题的 window_bg。
    virtual void begin(glm::vec4 clear_color) = 0;
    virtual void draw(const graphics::Drawable& drawable) = 0;
    virtual void end() = 0;

    // 离屏渲染一帧并读回像素（RGBA8；失败返回空）。
    virtual std::vector<unsigned char> capture(const graphics::Drawable& drawable,
                                               std::uint32_t width, std::uint32_t height) = 0;
};

// 把 RGBA8 像素写成 PNG（stb_image_write）；与具体后端无关。
bool write_png(const std::string& path, std::uint32_t width, std::uint32_t height,
               const std::vector<unsigned char>& rgba);
} // namespace xel::backend
