#pragma once

// #include "../events/event.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <functional>

namespace xel::backend
{
class Window
{
public:
    // using EventCallback = std::function<void(const xel::event::Event&)>;
    // void set_event_callback(const EventCallback& callback) { event_callback_ = std::move(callback); };

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
    GLFWwindow* get_glfw_window() { return window_; }

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
private:
    void init_window();

    int width_;
    int height_;
    bool framebuffer_resized_ = false;

    // EventCallback event_callback_;

    std::string window_name_;
    GLFWwindow* window_ = nullptr;
};
}