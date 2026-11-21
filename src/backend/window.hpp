#pragma once

// 项目只用 Vulkan，不使用 OpenGL：GLFW_INCLUDE_NONE 阻止 glfw3.h 拉入 <GL/gl.h>，
// 否则在没有 GL 开发包的机器（如 CI 的 ubuntu-latest）上会编译失败。
// CMake 侧也对 glfw 目标设置了同名 INTERFACE 定义，这里做一次守卫式补充。
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <functional>

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
    VkExtent2D get_extent()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
    VkExtent2D get_window_extent();
    std::pair<float, float> framebuffer_scale();
    bool was_window_resized() { return framebuffer_resized_; }
    void reset_window_resized_flag() { framebuffer_resized_ = false; }
    GLFWwindow* get_glfw_window() { return window_; }

    // 光标/坐标换算：窗口坐标 → framebuffer 物理像素（× DPR）。
    // 属于 UI 层，不在这里做。
    glm::vec2 cursor_pos();
    glm::vec2 window_to_framebuffer(glm::vec2 window_pos);
    glm::vec2 cursor_to_framebuffer();

    void set_framebuffer_size(int width, int height)
    {
        width_ = width;
        height_ = height;
        framebuffer_resized_ = true;
    }

private:
    void init_window();

    int width_;
    int height_;
    bool framebuffer_resized_ = false;

    std::string window_name_;
    GLFWwindow* window_ = nullptr;
};
}