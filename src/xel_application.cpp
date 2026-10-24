#include "xel_application.hpp"
#include "events/event.hpp"

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
    event.key = key;
    event.mods = mods;
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
