#include "ui_gallery.hpp"
#include "xel/version.hpp"
#include "xel_application.hpp"

#include <exception>
#include <format>
#include <iostream>
#include <string_view>

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
    auto gallery = xel::demo::build(*app.uisys.root());

    bool f1_prev = false;

    while (!app.window_.should_close()) {
        glfwPollEvents();

        const bool f1_pressed = glfwGetKey(app.window_.get_glfw_window(), GLFW_KEY_F1) == GLFW_PRESS;
        if (f1_pressed && !f1_prev) gallery.debug->toggle();
        f1_prev = f1_pressed;

        app.renderer_.begin();

        const glm::vec2 cursor = cursor_to_framebuffer(app.window_);
        gallery.update_tooltip(cursor);

        const auto& stats = app.renderer_.frame_stats();
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

        app.uisys.tick();
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
