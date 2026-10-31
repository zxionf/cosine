#include "ui_gallery.hpp"
#include "xel/version.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
glm::vec2 cursor_to_framebuffer(xel::backend::Window& window)
{
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window.get_glfw_window(), &x, &y);
    const auto [sx, sy] = window.framebuffer_scale();
    return {static_cast<float>(x) * sx, static_cast<float>(y) * sy};
}

int run()
{
    xel::Application app{1200, 800, "XEL UI Gallery"};

    // Image 纹理示例：内存生成 8×4 棋盘 + 渐变，上传到图片槽 0。
    std::uint32_t image_texture_id = 0;
    {
        constexpr int kTexW = 8;
        constexpr int kTexH = 4;
        std::vector<unsigned char> pixels(static_cast<std::size_t>(kTexW * kTexH * 4));
        for (int y = 0; y < kTexH; ++y) {
            for (int x = 0; x < kTexW; ++x) {
                const std::size_t i = static_cast<std::size_t>((y * kTexW + x) * 4);
                const bool check = ((x / 2) + (y / 2)) % 2 == 0;
                pixels[i + 0] = check ? 230 : 50;
                pixels[i + 1] = static_cast<unsigned char>(x * 255 / (kTexW - 1));
                pixels[i + 2] = static_cast<unsigned char>(y * 255 / (kTexH - 1));
                pixels[i + 3] = 255;
            }
        }
        if (app.renderer_.upload_texture(0, kTexW, kTexH, pixels.data())) image_texture_id = 1;
    }

    auto gallery = xel::demo::build(*app.uisys.root(), image_texture_id, {8.0f, 4.0f});
    gallery.ui_system = &app.uisys;

    bool f1_prev = false;
    bool open_dropdown_once = std::getenv("COSINE_GALLERY_DROPDOWN") != nullptr;

    while (!app.window_.should_close()) {
        glfwPollEvents();

        const bool f1_pressed = glfwGetKey(app.window_.get_glfw_window(), GLFW_KEY_F1) == GLFW_PRESS;
        if (f1_pressed && !f1_prev) gallery.debug->toggle();
        f1_prev = f1_pressed;

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        app.renderer_.begin();

        const auto& stats = app.renderer_.frame_stats();

        // 光标：窗口坐标 → framebuffer → UI 逻辑坐标（root 与命中测试都用后者）。
        const glm::vec2 cursor = cursor_to_framebuffer(app.window_) / app.ui_scale();
        gallery.update_tooltip(cursor, static_cast<float>(stats.frame_time_ms) / 1000.0f);

        gallery.stats->set_text(std::format(
            "fps: {:.1f}   frame: {:.2f} ms   draw calls: {}   vertices: {}   indices: {}",
            stats.fps, stats.frame_time_ms, stats.draw.draw_calls,
            stats.draw.vertices, stats.draw.indices));
        gallery.scroll_state->set_text(std::format(
            "scroll: {:.0f}, {:.0f}   content: {:.0f}x{:.0f}",
            gallery.scroll_both->scroll_offset_x(), gallery.scroll_both->scroll_offset(),
            gallery.scroll_both->content_size().x, gallery.scroll_both->content_size().y));

        if (gallery.debug->visible()) {
            gallery.debug->update(stats, app.uisys.hovered(), app.uisys.focused(),
                                  gallery.scroll_vertical->scroll_offset_vec());
        }

        const float dt = std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);
        app.uisys.tick(dt);

        // 冒烟钩子：到时正常关窗，确保析构路径（Vulkan 资源释放）被执行。
        if (const char* exit_after_env = std::getenv("COSINE_EXIT_AFTER")) {
            static float total_time = 0.0f;
            total_time += dt;
            if (total_time >= static_cast<float>(std::atof(exit_after_env))) {
                glfwSetWindowShouldClose(app.window_.get_glfw_window(), GLFW_TRUE);
            }
        }

        // 首帧布局完成后再应用待定滚动（build 时还没有 viewport 尺寸）。
        if (gallery.pending_scroll >= 0.0f) {
            if (gallery.current_section < gallery.sections.size()) {
                if (auto* view = gallery.sections[gallery.current_section]->scroll_view()) {
                    view->set_scroll_offset({0.0f, gallery.pending_scroll});
                }
            }
            gallery.pending_scroll = -1.0f;
        }

        if (open_dropdown_once) {
            open_dropdown_once = false;
            gallery.dropdown->open();
        }

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();
    }

    app.vulkan_context_.device().waitIdle();
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--version") {
            std::cout << xel::kProjectName << " " << xel::version_string() << '\n';
            return 0;
        }
    }

    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
