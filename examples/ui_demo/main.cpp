#include "ui_demo.hpp"
#include "xel/version.hpp"
#include "xel_application.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <string_view>

namespace
{
// 边沿检测：按住不放只触发一次。
struct KeyLatch
{
    int key = 0;
    bool prev = false;
    bool pressed(bool down)
    {
        const bool fired = down && !prev;
        prev = down;
        return fired;
    }
};

int run()
{
    xel::Application app{1200, 800, "XEL Text Lab"};
    auto lab = xel::demo::build(*app.uisys.root());

    KeyLatch f1{GLFW_KEY_F1};
    KeyLatch f2{GLFW_KEY_F2};
    KeyLatch f3{GLFW_KEY_F3};
    KeyLatch f4{GLFW_KEY_F4};
    KeyLatch f5{GLFW_KEY_F5};

    while (!app.window_.should_close()) {
        glfwPollEvents();

        auto* window = app.window_.get_glfw_window();
        if (f1.pressed(glfwGetKey(window, f1.key) == GLFW_PRESS)) lab.overlay->toggle();
        if (f2.pressed(glfwGetKey(window, f2.key) == GLFW_PRESS)) lab.toggle_glyph_boxes();
        if (f3.pressed(glfwGetKey(window, f3.key) == GLFW_PRESS)) lab.toggle_line_box();
        if (f4.pressed(glfwGetKey(window, f4.key) == GLFW_PRESS)) lab.toggle_advance();
        if (f5.pressed(glfwGetKey(window, f5.key) == GLFW_PRESS)) lab.toggle_pixel_snap();

        app.renderer_.begin();

        if (lab.overlay->visible()) {
            lab.overlay->update(app.renderer_.frame_stats(), app.uisys.hovered(),
                                app.uisys.focused(), {0.0f, 0.0f});
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
