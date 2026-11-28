#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <string_view>

namespace
{
// UI 光标概念 → 平台窗口光标（唯一的映射点，backend 不认识 ui）。
xel::backend::Window::Cursor to_window_cursor(xel::ui::CursorShape shape)
{
    using Cursor = xel::backend::Window::Cursor;
    switch (shape) {
        case xel::ui::CursorShape::ResizeEW:   return Cursor::ResizeEW;
        case xel::ui::CursorShape::ResizeNS:   return Cursor::ResizeNS;
        case xel::ui::CursorShape::ResizeNWSE: return Cursor::ResizeNWSE;
        case xel::ui::CursorShape::ResizeNESW: return Cursor::ResizeNESW;
        case xel::ui::CursorShape::Arrow:      break;
    }
    return Cursor::Arrow;
}

// COSINE_UI_SCALE：auto 跟随真实 DPR，或指定倍数；未设置时为 1.0。
float ui_scale_from_env(float dpr)
{
    const char* env = std::getenv("COSINE_UI_SCALE");
    if (env == nullptr) return 1.0f;
    const std::string_view value{env};
    if (value == "auto") return std::max(1.0f, dpr);
    return std::stof(std::string{value});
}

// 冒烟/导出用的到时退出（秒）；0 表示不启用。
float exit_after_from_env()
{
    const char* env = std::getenv("COSINE_EXIT_AFTER");
    return env != nullptr ? static_cast<float>(std::atof(env)) : 0.0f;
}
} // namespace

namespace xel
{
Application::Application(Options options)
: window_{options.width, options.height, options.title},
  vulkan_context_{window_},
  swap_chain_{vulkan_context_, window_},
  renderer_{vulkan_context_, swap_chain_},
  ui_{}
{
    // 宿主能力一次性注入：字体图集、系统光标、渲染缩放。
    ui_.set_font_atlas(&renderer_.font_atlas());
    ui_.bind_host({
        [this](ui::CursorShape shape) { window_.set_cursor(to_window_cursor(shape)); },
        // 一个比例同时作用于渲染投影与字体栅格化（后端自己扇出）。
        [this](float scale) { renderer_.set_scale(scale); },
    });

    // 显示比例的**唯一事实**：dpr 来自窗口，ui_scale 来自环境；之后每次变化都走
    // apply_display_scale() 一次扇出（输入换算 / root 尺寸 / 渲染 / 字体）。
    const float dpr = window_.framebuffer_scale().first;
    scale_ = graphics::DisplayScale::make(dpr, ui_scale_from_env(dpr));

    const auto extent = window_.get_extent();
    ui_.set_viewport({static_cast<float>(extent.width), static_cast<float>(extent.height)});
    apply_display_scale();
}

Application::~Application() = default;

void Application::set_ui_scale(float scale)
{
    scale_ = scale_.with_ui_scale(scale);
    apply_display_scale();
}

void Application::apply_display_scale()
{
    // 一份显示比例，两处下发：UI 运行时（root 尺寸 + 渲染/字体）、输入翻译。
    ui_.set_display_scale(scale_);
    window_.set_display_scale(scale_);
}

void Application::poll_events()
{
    window_.poll();
    for (const auto& event : window_.events()) ui_.dispatch(event);
    if (!window_.has_pending_viewport()) return;

    ui_.set_viewport(window_.take_viewport());
    // 跨屏移动/系统缩放下 dpr 会变：视图变化时重取一次，仍只在这一处更新。
    const float dpr = window_.framebuffer_scale().first;
    if (dpr > 0.0f && dpr != scale_.dpr) {
        scale_ = scale_.with_dpr(dpr);
        apply_display_scale();
    }
}

float Application::frame_dt() const
{
    const float seconds = static_cast<float>(renderer_.frame_stats().frame_time_ms) / 1000.0f;
    return std::clamp(seconds, 0.0f, 0.1f);
}

int Application::run(const FrameFn& frame)
{
    const float exit_after = exit_after_from_env();
    float total_time = 0.0f;

    while (!quit_requested()) {
        poll_events();

        // begin 会更新 frame_stats，所以 dt 在它之后取。
        gfx().begin(ui_.context()->theme().window_bg);
        const float dt = frame_dt();

        if (frame) frame(dt);
        ui_.tick(dt);

        gfx().draw(ui_.root());
        gfx().end();

        total_time += dt;
        if (exit_after > 0.0f && total_time >= exit_after) request_quit();
    }

    vulkan_context_.device().waitIdle();
    return 0;
}

bool Application::save_png(const std::string& path, std::uint32_t width, std::uint32_t height)
{
    // 导出尺寸与窗口 DPI 无关：root 与离屏目标使用同一逻辑尺寸。
    auto& root = ui_.root();
    root.set_size({static_cast<float>(width), static_cast<float>(height)});
    root.mark_layout_dirty();
    root.layout();
    root.clear_layout_dirty();

    const auto pixels = gfx().capture(root, width, height);
    return backend::write_png(path, width, height, pixels);
}
} // namespace xel
