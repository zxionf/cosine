#include "xel_application.hpp"
#include "events/event.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>


namespace
{
xel::event::KeyCode map_key(int key)
{
    switch (key) {
        case GLFW_KEY_TAB:        return xel::event::KeyCode::Tab;
        case GLFW_KEY_ENTER:      return xel::event::KeyCode::Enter;
        case GLFW_KEY_KP_ENTER:   return xel::event::KeyCode::KeypadEnter;
        case GLFW_KEY_SPACE:      return xel::event::KeyCode::Space;
        case GLFW_KEY_BACKSPACE:  return xel::event::KeyCode::Backspace;
        case GLFW_KEY_DELETE:     return xel::event::KeyCode::Delete;
        case GLFW_KEY_LEFT:       return xel::event::KeyCode::Left;
        case GLFW_KEY_RIGHT:      return xel::event::KeyCode::Right;
        case GLFW_KEY_UP:         return xel::event::KeyCode::Up;
        case GLFW_KEY_DOWN:       return xel::event::KeyCode::Down;
        case GLFW_KEY_HOME:       return xel::event::KeyCode::Home;
        case GLFW_KEY_END:        return xel::event::KeyCode::End;
        case GLFW_KEY_PAGE_UP:    return xel::event::KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN:  return xel::event::KeyCode::PageDown;
        case GLFW_KEY_ESCAPE:     return xel::event::KeyCode::Escape;
        case GLFW_KEY_A:          return xel::event::KeyCode::A;
        default:                  return xel::event::KeyCode::Unknown;
    }
}

xel::event::Modifier map_mods(int mods)
{
    auto result = xel::event::Modifier::None;
    if ((mods & GLFW_MOD_SHIFT) != 0)   result = result | xel::event::Modifier::Shift;
    if ((mods & GLFW_MOD_CONTROL) != 0) result = result | xel::event::Modifier::Control;
    if ((mods & GLFW_MOD_ALT) != 0)     result = result | xel::event::Modifier::Alt;
    if ((mods & GLFW_MOD_SUPER) != 0)   result = result | xel::event::Modifier::Super;
    return result;
}

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
: window_{window_w, window_h, title}, vulkan_context_{window_}, swap_chain_{vulkan_context_, window_}, renderer_{vulkan_context_, swap_chain_}, uisys{std::make_unique<ui::UIRoot>(0.0f, 0.0f, 0.0f, 0.0f)}
{
    uisys.root()->context()->font_atlas = &renderer_.font_atlas();

    // 视图尺寸与光标都由这里推给 UI：UISystem 不认识 backend::Window。
    const auto extent = window_.get_extent();
    uisys.set_viewport({static_cast<float>(extent.width), static_cast<float>(extent.height)});
    uisys.bind_host({[this](ui::CursorShape shape) { window_.set_cursor(to_window_cursor(shape)); }});

    glfwSetWindowUserPointer(window_.get_glfw_window(), this);

    glfwSetFramebufferSizeCallback(window_.get_glfw_window(), framebuffer_resize_callback);
    glfwSetMouseButtonCallback(window_.get_glfw_window(), mouse_button_callback);
    glfwSetKeyCallback(window_.get_glfw_window(), key_callback);
    glfwSetCharCallback(window_.get_glfw_window(), char_callback);
    glfwSetCursorPosCallback(window_.get_glfw_window(), cursor_pos_callback);
    glfwSetScrollCallback(window_.get_glfw_window(), scroll_callback);

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
    ui_scale_ = std::clamp(scale, 0.25f, 4.0f);
    renderer_.set_ui_scale(ui_scale_);
    uisys.set_ui_scale(ui_scale_);
    renderer_.set_font_pixel_ratio(ui_scale_);
}

Application::~Application()
{}

void Application::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->window_.set_framebuffer_size(width, height);
    app->uisys.set_viewport({static_cast<float>(width), static_cast<float>(height)});
}

void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    event::InputEvent event;
    event.type = action == GLFW_PRESS
        ? event::InputEvent::Type::MouseButtonDown
        : event::InputEvent::Type::MouseButtonUp;
    event.mouse_button = button;
    // 鼠标事件也带修饰键：Shift/Ctrl 追加选择靠它。
    event.mods = map_mods(mods);

    event.mouse_pos = app->uisys.framebuffer_to_logical(app->window_.cursor_to_framebuffer());

    app->uisys.dispatch(event);
}

void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    if (action != GLFW_PRESS && action != GLFW_RELEASE) return;

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = action == GLFW_PRESS
        ? event::InputEvent::Type::KeyDown
        : event::InputEvent::Type::KeyUp;
    event.key = map_key(key);
    event.mods = map_mods(mods);
    app->uisys.dispatch(event);
}

void Application::char_callback(GLFWwindow* window, unsigned int codepoint)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = event::InputEvent::Type::CharInput;
    event.codepoint = static_cast<char32_t>(codepoint);
    app->uisys.dispatch(event);
}

void Application::cursor_pos_callback(GLFWwindow* window, double mx, double my)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseMove;
    event.mouse_pos = app->uisys.framebuffer_to_logical(
        app->window_.window_to_framebuffer({static_cast<float>(mx), static_cast<float>(my)}));
    //e.mouse_delta = e.mouse_pos - app->last_mouse_;
    //app->last_mouse_ = e.mouse_pos;
    app->uisys.dispatch(event);
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseScroll;
    event.scroll_delta_x = static_cast<float>(xoffset);
    event.scroll_delta = static_cast<float>(yoffset);
    event.mouse_pos = app->uisys.framebuffer_to_logical(app->window_.cursor_to_framebuffer());
    app->uisys.dispatch(event);
}

} // namespace xel
