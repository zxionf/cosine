#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
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
} // namespace

namespace xel
{
Application::Application(int window_w, int window_h, const std::string& title)
: window_{window_w, window_h, title}, vulkan_context_{window_}, swap_chain_{vulkan_context_, window_}, renderer_{vulkan_context_, swap_chain_}, ui{}
{
    // 宿主能力一次性注入：字体图集、系统光标、渲染缩放（DPI 扇出）。
    ui.set_font_atlas(&renderer_.font_atlas());
    ui.bind_host({
        [this](ui::CursorShape shape) { window_.set_cursor(to_window_cursor(shape)); },
        [this](float scale) {
            renderer_.set_ui_scale(scale);
            renderer_.set_font_pixel_ratio(scale);
        },
    });

    // 视图尺寸也由宿主推：UISystem 不认识 backend::Window。
    const auto extent = window_.get_extent();
    ui.set_viewport({static_cast<float>(extent.width), static_cast<float>(extent.height)});

    // DPI 缩放：COSINE_UI_SCALE=auto 跟随真实 DPR（framebuffer/window），
    // 或指定 0.25–4 的倍数；默认 1.0，与旧行为一致。
    float scale = 1.0f;
    if (const char* env = std::getenv("COSINE_UI_SCALE")) {
        const std::string_view value{env};
        scale = value == "auto"
            ? std::max(1.0f, window_.framebuffer_scale().first)
            : std::stof(std::string{value});
    }
    set_ui_scale(scale);
}

void Application::set_ui_scale(float scale)
{
    ui.set_ui_scale(scale);
    // 输入换算与 DPI 同源：窗口坐标 → 逻辑坐标的那一步也在这里收口。
    window_.set_ui_scale(ui.ui_scale());
}

void Application::poll_events()
{
    window_.poll();
    for (const auto& event : window_.events()) ui.dispatch(event);
    if (window_.has_pending_viewport()) ui.set_viewport(window_.take_viewport());
}

Application::~Application() = default;
} // namespace xel
