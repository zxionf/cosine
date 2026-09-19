#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace xel::backend
{
    class Window
    {
    public:
        Window(int width, int height, std::string name);
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        bool should_close() { return glfwWindowShouldClose(window_); }
        VkExtent2D get_extent() { return { static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)}; }
        bool was_window_resized() { return framebuffer_resized_; }
        void reset_window_resized_flag() { framebuffer_resized_ = false; }
        GLFWwindow* getGLFWwindow() { return window_; }

        void create_window_surface(VkInstance instance, VkSurfaceKHR *surface);
    private:
        static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
        void init_window();

        int width_;
        int height_;
        bool framebuffer_resized_ = false;

        std::string window_name_;
        GLFWwindow* window_ = nullptr;
    };
}