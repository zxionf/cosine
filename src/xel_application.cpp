#include "xel_application.hpp"
#include "events/event.hpp"


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
} // namespace

namespace
{
glm::vec2 cursor_to_framebuffer(xel::backend::Window& window, double x, double y)
{
    const auto [sx, sy] = window.framebuffer_scale();
    return {static_cast<float>(x) * sx, static_cast<float>(y) * sy};
}
} // namespace

namespace xel
{
Application::Application(int window_w, int window_h, const std::string& title)
: window_{window_w, window_h, title}, vulkan_context_{window_}, swap_chain_{vulkan_context_, window_}, renderer_{vulkan_context_, swap_chain_}, uisys{window_}
{
    uisys.root()->context()->font_atlas = &renderer_.font_atlas();

    glfwSetWindowUserPointer(window_.get_glfw_window(), this);

    glfwSetFramebufferSizeCallback(window_.get_glfw_window(), framebuffer_resize_callback);
    glfwSetMouseButtonCallback(window_.get_glfw_window(), mouse_button_callback);
    glfwSetKeyCallback(window_.get_glfw_window(), key_callback);
    glfwSetCharCallback(window_.get_glfw_window(), char_callback);
    glfwSetCursorPosCallback(window_.get_glfw_window(), cursor_pos_callback);
    glfwSetScrollCallback(window_.get_glfw_window(), scroll_callback);
}

Application::~Application()
{}

void Application::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->window_.set_framebuffer_size(width, height);
}

void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    event::InputEvent event;
    event.type = action == GLFW_PRESS
        ? event::InputEvent::Type::MouseButtonDown
        : event::InputEvent::Type::MouseButtonUp;
    event.mouse_button = button;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    event.mouse_pos = cursor_to_framebuffer(app->window_, mx, my);

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
    event.mouse_pos = cursor_to_framebuffer(app->window_, mx, my);
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
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    event.mouse_pos = cursor_to_framebuffer(app->window_, mx, my);
    app->uisys.dispatch(event);
}

} // namespace xel
