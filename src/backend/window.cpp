#include "window.hpp"

#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace xel::backend
{
namespace
{
event::KeyCode map_key(int key)
{
    switch (key) {
        case GLFW_KEY_TAB:        return event::KeyCode::Tab;
        case GLFW_KEY_ENTER:      return event::KeyCode::Enter;
        case GLFW_KEY_KP_ENTER:   return event::KeyCode::KeypadEnter;
        case GLFW_KEY_SPACE:      return event::KeyCode::Space;
        case GLFW_KEY_BACKSPACE:  return event::KeyCode::Backspace;
        case GLFW_KEY_DELETE:     return event::KeyCode::Delete;
        case GLFW_KEY_LEFT:       return event::KeyCode::Left;
        case GLFW_KEY_RIGHT:      return event::KeyCode::Right;
        case GLFW_KEY_UP:         return event::KeyCode::Up;
        case GLFW_KEY_DOWN:       return event::KeyCode::Down;
        case GLFW_KEY_HOME:       return event::KeyCode::Home;
        case GLFW_KEY_END:        return event::KeyCode::End;
        case GLFW_KEY_PAGE_UP:    return event::KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN:  return event::KeyCode::PageDown;
        case GLFW_KEY_ESCAPE:     return event::KeyCode::Escape;
        case GLFW_KEY_F1:         return event::KeyCode::F1;
        case GLFW_KEY_F2:         return event::KeyCode::F2;
        case GLFW_KEY_F3:         return event::KeyCode::F3;
        case GLFW_KEY_F4:         return event::KeyCode::F4;
        case GLFW_KEY_F5:         return event::KeyCode::F5;
        case GLFW_KEY_F6:         return event::KeyCode::F6;
        case GLFW_KEY_F7:         return event::KeyCode::F7;
        case GLFW_KEY_F8:         return event::KeyCode::F8;
        case GLFW_KEY_F9:         return event::KeyCode::F9;
        case GLFW_KEY_F10:        return event::KeyCode::F10;
        case GLFW_KEY_F11:        return event::KeyCode::F11;
        case GLFW_KEY_F12:        return event::KeyCode::F12;
        case GLFW_KEY_A:          return event::KeyCode::A;
        default:                  return event::KeyCode::Unknown;
    }
}

// KeyCode → GLFW 键码（按键电平查询用，与 map_key 互为逆映射）。
int glfw_key_of(event::KeyCode key)
{
    switch (key) {
        case event::KeyCode::Tab:         return GLFW_KEY_TAB;
        case event::KeyCode::Enter:       return GLFW_KEY_ENTER;
        case event::KeyCode::KeypadEnter: return GLFW_KEY_KP_ENTER;
        case event::KeyCode::Space:       return GLFW_KEY_SPACE;
        case event::KeyCode::Backspace:   return GLFW_KEY_BACKSPACE;
        case event::KeyCode::Delete:      return GLFW_KEY_DELETE;
        case event::KeyCode::Left:        return GLFW_KEY_LEFT;
        case event::KeyCode::Right:       return GLFW_KEY_RIGHT;
        case event::KeyCode::Up:          return GLFW_KEY_UP;
        case event::KeyCode::Down:        return GLFW_KEY_DOWN;
        case event::KeyCode::Home:        return GLFW_KEY_HOME;
        case event::KeyCode::End:         return GLFW_KEY_END;
        case event::KeyCode::PageUp:      return GLFW_KEY_PAGE_UP;
        case event::KeyCode::PageDown:    return GLFW_KEY_PAGE_DOWN;
        case event::KeyCode::Escape:      return GLFW_KEY_ESCAPE;
        case event::KeyCode::F1:          return GLFW_KEY_F1;
        case event::KeyCode::F2:          return GLFW_KEY_F2;
        case event::KeyCode::F3:          return GLFW_KEY_F3;
        case event::KeyCode::F4:          return GLFW_KEY_F4;
        case event::KeyCode::F5:          return GLFW_KEY_F5;
        case event::KeyCode::F6:          return GLFW_KEY_F6;
        case event::KeyCode::F7:          return GLFW_KEY_F7;
        case event::KeyCode::F8:          return GLFW_KEY_F8;
        case event::KeyCode::F9:          return GLFW_KEY_F9;
        case event::KeyCode::F10:         return GLFW_KEY_F10;
        case event::KeyCode::F11:         return GLFW_KEY_F11;
        case event::KeyCode::F12:         return GLFW_KEY_F12;
        case event::KeyCode::A:           return GLFW_KEY_A;
        case event::KeyCode::Unknown:     break;
    }
    return GLFW_KEY_UNKNOWN;
}

event::Modifier map_mods(int mods)
{
    auto result = event::Modifier::None;
    if ((mods & GLFW_MOD_SHIFT) != 0)   result = result | event::Modifier::Shift;
    if ((mods & GLFW_MOD_CONTROL) != 0) result = result | event::Modifier::Control;
    if ((mods & GLFW_MOD_ALT) != 0)     result = result | event::Modifier::Alt;
    if ((mods & GLFW_MOD_SUPER) != 0)   result = result | event::Modifier::Super;
    return result;
}
} // namespace

Window::Window(int width, int height, std::string name)
: width_{width}, height_{height}, window_name_{name}
{
    init_window();
    init_callbacks();
}

Window::~Window()
{
    for (auto& [shape, cursor] : cursor_cache_) {
        glfwDestroyCursor(cursor);
    }
    cursor_cache_.clear();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Window::init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    window_ = glfwCreateWindow(width_, height_, window_name_.c_str(), nullptr, nullptr);
}

// ── 回调桥：user pointer 指向 Window 自己 ──────────────────────────
Window* Window::from_glfw(GLFWwindow* window)
{
    return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void Window::init_callbacks()
{
    glfwSetWindowUserPointer(window_, this);

    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* w, int width, int height) {
        from_glfw(w)->on_framebuffer_size(width, height);
    });
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        from_glfw(w)->on_mouse_button(button, action, mods);
    });
    glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int /*scancode*/, int action, int mods) {
        from_glfw(w)->on_key(key, action, mods);
    });
    glfwSetCharCallback(window_, [](GLFWwindow* w, unsigned int codepoint) {
        from_glfw(w)->on_char(codepoint);
    });
    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double mx, double my) {
        from_glfw(w)->on_cursor_pos(mx, my);
    });
    glfwSetScrollCallback(window_, [](GLFWwindow* w, double xoffset, double yoffset) {
        from_glfw(w)->on_scroll(xoffset, yoffset);
    });
}

// ── 平台事件 → InputEvent（坐标在翻译期就换成 UI 逻辑坐标）───────────
void Window::on_framebuffer_size(int width, int height)
{
    set_framebuffer_size(width, height);
}

void Window::on_mouse_button(int button, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE) return;
    event::InputEvent event;
    event.type = action == GLFW_PRESS
        ? event::InputEvent::Type::MouseButtonDown
        : event::InputEvent::Type::MouseButtonUp;
    event.mouse_button = button;
    event.mods = map_mods(mods); // 鼠标事件也带修饰键：Shift/Ctrl 追加选择靠它。
    event.mouse_pos = window_to_logical(cursor_pos());
    pending_events_.push_back(event);
}

void Window::on_key(int key, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE) return;
    event::InputEvent event;
    event.type = action == GLFW_PRESS
        ? event::InputEvent::Type::KeyDown
        : event::InputEvent::Type::KeyUp;
    event.key = map_key(key);
    event.mods = map_mods(mods);
    pending_events_.push_back(event);
}

void Window::on_char(unsigned int codepoint)
{
    event::InputEvent event;
    event.type = event::InputEvent::Type::CharInput;
    event.codepoint = static_cast<char32_t>(codepoint);
    pending_events_.push_back(event);
}

void Window::on_cursor_pos(double mx, double my)
{
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseMove;
    event.mouse_pos = window_to_logical(
        {static_cast<float>(mx), static_cast<float>(my)});
    pending_events_.push_back(event);
}

void Window::on_scroll(double xoffset, double yoffset)
{
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseScroll;
    event.scroll_delta_x = static_cast<float>(xoffset);
    event.scroll_delta = static_cast<float>(yoffset);
    event.mouse_pos = window_to_logical(cursor_pos());
    pending_events_.push_back(event);
}

// ── EventSource ───────────────────────────────────────────────────
void Window::poll()
{
    pending_events_.clear();
    glfwPollEvents();
}

glm::vec2 Window::take_viewport()
{
    viewport_pending_ = false;
    return viewport_;
}

bool Window::quit_requested() const
{
    return glfwWindowShouldClose(window_) != 0;
}

void Window::request_quit()
{
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

bool Window::key_down(event::KeyCode key) const
{
    const int glfw_key = glfw_key_of(key);
    if (glfw_key == GLFW_KEY_UNKNOWN) return false;
    return glfwGetKey(window_, glfw_key) == GLFW_PRESS;
}

// ── 尺寸 / 坐标 ───────────────────────────────────────────────────
void Window::set_framebuffer_size(int width, int height)
{
    width_ = width;
    height_ = height;
    framebuffer_resized_ = true;
    viewport_ = {static_cast<float>(width), static_cast<float>(height)};
    viewport_pending_ = width > 0 && height > 0;
}

VkExtent2D Window::get_extent() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

VkExtent2D Window::get_window_extent() const
{
    int width = 0;
    int height = 0;
    glfwGetWindowSize(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

std::pair<float, float> Window::framebuffer_scale() const
{
    const VkExtent2D framebuffer = get_extent();
    const VkExtent2D window = get_window_extent();
    if (window.width == 0 || window.height == 0) return {1.0f, 1.0f};
    return {
        static_cast<float>(framebuffer.width) / static_cast<float>(window.width),
        static_cast<float>(framebuffer.height) / static_cast<float>(window.height)
    };
}

glm::vec2 Window::cursor_pos()
{
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

void Window::set_cursor(Cursor cursor)
{
    int shape = GLFW_ARROW_CURSOR;
    switch (cursor) {
        case Cursor::Arrow:      shape = GLFW_ARROW_CURSOR; break;
        case Cursor::ResizeEW:   shape = GLFW_RESIZE_EW_CURSOR; break;
        case Cursor::ResizeNS:   shape = GLFW_RESIZE_NS_CURSOR; break;
        case Cursor::ResizeNWSE: shape = GLFW_RESIZE_NWSE_CURSOR; break;
        case Cursor::ResizeNESW: shape = GLFW_RESIZE_NESW_CURSOR; break;
    }
    auto it = cursor_cache_.find(shape);
    if (it == cursor_cache_.end()) {
        it = cursor_cache_.emplace(shape, glfwCreateStandardCursor(shape)).first;
    }
    glfwSetCursor(window_, it->second);
}
} // namespace xel::backend
