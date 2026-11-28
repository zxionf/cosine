#pragma once

#include "backend/window.hpp"
#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/renderer.hpp"
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
    float ui_scale() const { return ui.ui_scale(); }
    // 转发到 UiRuntime：唯一扇出点（UISystem / renderer / 字体栅格化 / 输入换算）。
    void set_ui_scale(float scale);

    // 一帧的事件阶段：拉取平台事件 → 翻译好的输入转给 UI → 应用视图尺寸变化。
    // 主循环用它替代平台事件轮询，应用层不再接触平台 API。
    void poll_events();

    Application(int window_w, int window_h, const std::string& title);
    ~Application();
};
} // namespace xel
