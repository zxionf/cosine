#include "xel_application.hpp"
#include "events/event.hpp"

namespace xel
{
Application::Application(int window_w, int window_h, const std::string& title)
: window_{window_w, window_h, title}, vulkan_context_{window_}, swap_chain_{vulkan_context_, window_}, renderer_{vulkan_context_, swap_chain_}, uisys{window_}
{
    glfwSetWindowUserPointer(window_.get_glfw_window(), this);

    glfwSetFramebufferSizeCallback(window_.get_glfw_window(), framebuffer_resize_callback);
    glfwSetMouseButtonCallback(window_.get_glfw_window(), mouse_button_callback);
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
    event.mouse_pos = {static_cast<float>(mx), static_cast<float>(my)};

    app->uisys.dispatch(event);
}

void Application::cursor_pos_callback(GLFWwindow* window, double mx, double my)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseMove;
    event.mouse_pos = {static_cast<float>(mx), static_cast<float>(my)};
    //e.mouse_delta = e.mouse_pos - app->last_mouse_;
    //app->last_mouse_ = e.mouse_pos;
    app->uisys.dispatch(event);
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    event::InputEvent event;
    event.type = event::InputEvent::Type::MouseScroll;
    event.scroll_delta = static_cast<float>(yoffset);
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    event.mouse_pos = {static_cast<float>(mx), static_cast<float>(my)};
    app->uisys.dispatch(event);
}

} // namespace xel