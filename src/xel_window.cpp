#include "xel_window.hpp"

#include <stdexcept>

namespace xel
{
    XelWindow::XelWindow(int width, int height, std::string name) : width{width}, height{height}, windowName{name}
    {
        initWindow();
    }

    XelWindow::~XelWindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void XelWindow::initWindow()
    { 
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void XelWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface");
        }
    }

    void XelWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto xelWindow = reinterpret_cast<XelWindow *>(glfwGetWindowUserPointer(window));
        xelWindow->framebufferResized = true;
        xelWindow->width = width;
        xelWindow->height = height;
    }
}