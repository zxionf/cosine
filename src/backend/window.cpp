#include "window.hpp"

#include <stdexcept>
#include <iostream>

namespace xel::backend
{
Window::Window(int width, int height, std::string name) : width_{width}, height_{height}, window_name_{name}
{
    init_window();
}

Window::~Window()
{
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

    // glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

    // glfwSetWindowOpacity(window_, 0.5);

    GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    glfwSetCursor(window_, cursor);
}
}
namespace xel::backend
{
VkExtent2D Window::get_window_extent()
{
    int width = 0;
    int height = 0;
    glfwGetWindowSize(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

std::pair<float, float> Window::framebuffer_scale()
{
    const VkExtent2D framebuffer = get_extent();
    const VkExtent2D window = get_window_extent();
    if (window.width == 0 || window.height == 0) return {1.0f, 1.0f};
    return {
        static_cast<float>(framebuffer.width) / static_cast<float>(window.width),
        static_cast<float>(framebuffer.height) / static_cast<float>(window.height)
    };
}
} // namespace xel::backend
