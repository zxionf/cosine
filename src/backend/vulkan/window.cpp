#include "window.hpp"

#include <stdexcept>

namespace xel::backend::vulkan
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
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

        // glfwSetWindowOpacity(window_, 0.5);

        GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        glfwSetCursor(window_, cursor);
    }

    void Window::framebuffer_resize_callback(GLFWwindow *glfwWindow, int width, int height)
    {
        auto xelWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));
        xelWindow->framebuffer_resized_ = true;
        xelWindow->width_ = width;
        xelWindow->height_ = height;
    }
}