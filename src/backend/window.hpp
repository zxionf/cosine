#pragma once

// 项目只用 Vulkan，不使用 OpenGL：GLFW_INCLUDE_NONE 阻止 glfw3.h 拉入 <GL/gl.h>，
// 否则在没有 GL 开发包的机器（如 CI 的 ubuntu-latest）上会编译失败。
// CMake 侧也对 glfw 目标设置了同名 INTERFACE 定义，这里做一次守卫式补充。
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "events/event_source.hpp"
#include "graphics/display_scale.hpp"

#include <glm/glm.hpp>

#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xel::backend
{
// 平台窗口 + 输入源。
//
// GLFW 只出现在这个类里：窗口创建、光标、以及「GLFW 回调 → InputEvent」的翻译。
// 对外只暴露 event::EventSource 的接口（poll/events/viewport/quit）+ 渲染需要的
// extent / framebuffer_scale / Cursor，因此上层不需要认识任何 GLFW 常量。
class Window final : public event::EventSource
{
public:
    Window(int width, int height, std::string name);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // ── 渲染侧需要的窗口信息 ──────────────────────────────────────
    VkExtent2D get_extent() const;
    VkExtent2D get_window_extent() const;
    std::pair<float, float> framebuffer_scale() const;
    bool was_window_resized() const { return framebuffer_resized_; }
    void reset_window_resized_flag() { framebuffer_resized_ = false; }
    GLFWwindow* get_glfw_window() { return window_; }

    // ── 坐标换算 ─────────────────────────────────────────────────
    // 显示比例（dpr + ui_scale）由宿主注入：输入事件在翻译时就换成 UI 逻辑坐标，
    // 消费者不再各自换算，也不会在每个事件里重算 framebuffer/window 尺寸。
    void set_display_scale(const graphics::DisplayScale& scale) { scale_ = scale; }
    const graphics::DisplayScale& display_scale() const { return scale_; }

    // 窗口坐标 → UI 逻辑坐标（= 窗口坐标 × dpr ÷ ui_scale）。
    glm::vec2 window_to_logical(glm::vec2 window_pos) const
    {
        return scale_.window_to_logical(window_pos);
    }
    // 当前光标位置的窗口坐标。
    glm::vec2 cursor_pos();

    // ── event::EventSource ──────────────────────────────────────
    void poll() override;
    std::span<const event::InputEvent> events() const override { return pending_events_; }
    bool has_pending_viewport() const override { return viewport_pending_; }
    glm::vec2 take_viewport() override;
    bool quit_requested() const override;
    void request_quit() override;
    bool key_down(event::KeyCode key) const override;

    // 系统光标形状（UI 层用，避免把 GLFW 常量泄漏到 ui）。
    enum class Cursor
    {
        Arrow,
        ResizeEW,     // 左右
        ResizeNS,     // 上下
        ResizeNWSE,   // 左上 ↔ 右下
        ResizeNESW,   // 右上 ↔ 左下
    };
    void set_cursor(Cursor cursor);

    // 尺寸变更登记（窗口回调或外部强制尺寸时调用）。
    void set_framebuffer_size(int width, int height);

private:
    void init_window();
    void init_callbacks();
    // 回调桥：从 GLFWwindow 的 user pointer 取回 Window（不再 static_cast 到 app）。
    static Window* from_glfw(GLFWwindow* window);

    void on_framebuffer_size(int width, int height);
    void on_mouse_button(int button, int action, int mods);
    void on_key(int key, int action, int mods);
    void on_char(unsigned int codepoint);
    void on_cursor_pos(double mx, double my);
    void on_scroll(double xoffset, double yoffset);

    int width_;
    int height_;
    bool framebuffer_resized_ = false;
    graphics::DisplayScale scale_{};

    std::string window_name_;
    GLFWwindow* window_ = nullptr;
    std::unordered_map<int, GLFWcursor*> cursor_cache_;

    std::vector<event::InputEvent> pending_events_;
    bool viewport_pending_ = false;
    glm::vec2 viewport_{0.0f, 0.0f};
};
} // namespace xel::backend
