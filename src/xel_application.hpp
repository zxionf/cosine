#pragma once

#include "backend/window.hpp"
#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/renderer.hpp"
#include "graphics/display_scale.hpp"
#include "ui/system/ui_runtime.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace xel
{
// ── 组合根：装配窗口 / Vulkan / UI，并提供一个标准主循环 ─────────────
//
// 它只做编排，不含业务：
//   run(frame) 每帧 = 拉取事件 → 开始渲染帧 → frame(dt) → UI tick → 绘制 → 提交
//
// 依赖都收在私有成员里，对外只给窄接口：ui() / gfx() / renderer() / window()。
// 因此 examples 与测试既不需要重复主循环，也拿不到可随意改写的装配状态。
class Application
{
public:
    struct Options
    {
        int width = 1280;
        int height = 840;
        std::string title = "xel";
    };

    explicit Application(Options options);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // ── 窄接口 ────────────────────────────────────────────────────
    ui::UiRuntime& ui() { return ui_; }
    const ui::UiRuntime& ui() const { return ui_; }
    // 渲染后端接口（examples / 测试只用它，不依赖 Vulkan）。
    backend::RenderBackend& gfx() { return renderer_; }
    const backend::RenderBackend& gfx() const { return renderer_; }
    // Vulkan 专用面：图片槽上传、设备等待等。
    backend::vulkan::Renderer& renderer() { return renderer_; }
    backend::Window& window() { return window_; }
    const backend::Window& window() const { return window_; }

    const graphics::DisplayScale& display_scale() const { return scale_; }
    float ui_scale() const { return scale_.ui_scale; }
    // 设置用户比例：夹取后一次性扇出（输入换算 / root 尺寸 / 渲染 / 字体）。
    void set_ui_scale(float scale);

    // ── 主循环 ────────────────────────────────────────────────────
    // frame(dt) 为本帧业务逻辑（dt 秒，来自上一帧耗时，夹到 [0, 0.1]）。
    // 返回进程退出码（当前恒为 0）；退出前等待 GPU 空闲。
    using FrameFn = std::function<void(float dt)>;
    int run(const FrameFn& frame = {});

    // ── 单步能力（离屏导出 / 测试）─────────────────────────────────
    // 拉取平台事件并派发；应用视图尺寸变化。
    void poll_events();
    // 把当前 UI 画面离屏渲染并写成 PNG（尺寸为逻辑尺寸，与窗口 DPI 无关）。
    bool save_png(const std::string& path, std::uint32_t width, std::uint32_t height);

    bool quit_requested() const { return window_.quit_requested(); }
    void request_quit() { window_.request_quit(); }

private:
    void apply_display_scale();
    // 上一帧耗时（秒），夹到 [0, 0.1]；首帧为 0。
    float frame_dt() const;

    backend::Window window_;
    backend::vulkan::VulkanContext vulkan_context_;
    backend::vulkan::SwapChain swap_chain_;
    backend::vulkan::Renderer renderer_;
    ui::UiRuntime ui_;
    graphics::DisplayScale scale_{};
};
} // namespace xel
