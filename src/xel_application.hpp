#pragma once

#include "backend/window.hpp"
#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/renderer.hpp"
#include "graphics/display_scale.hpp"
#include "ui/system/ui_runtime.hpp"

namespace xel
{
class Application
{
public:
    backend::Window window_;
    backend::vulkan::VulkanContext vulkan_context_;
    backend::vulkan::SwapChain swap_chain_;
    backend::vulkan::Renderer renderer_;
    // UI 运行时（root + 调度 + DPI + 宿主注入）；不含任何平台或渲染依赖。
    ui::UiRuntime ui;

    // 渲染后端接口：examples / 快照测试只依赖它，不依赖 Vulkan。
    backend::RenderBackend& gfx() { return renderer_; }

    // DPI：逻辑像素 → framebuffer 像素的缩放比（COSINE_UI_SCALE，默认 1.0）。
    float ui_scale() const { return scale_.ui_scale; }
    // 当前显示比例（dpr + ui_scale 的唯一事实）。
    const graphics::DisplayScale& display_scale() const { return scale_; }
    // 设置用户比例：夹取后一次性扇出（UISystem / 输入换算 / renderer / 字体栅格化）。
    void set_ui_scale(float scale);

    // 一帧的事件阶段：拉取平台事件 → 翻译好的输入转给 UI → 应用视图尺寸变化。
    // 主循环用它替代平台事件轮询，应用层不再接触平台 API。
    void poll_events();

    Application(int window_w, int window_h, const std::string& title);
    ~Application();

private:
    void apply_display_scale();

    graphics::DisplayScale scale_{};
};
} // namespace xel
