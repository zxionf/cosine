#include "ui_demo.hpp"
#include "xel/version.hpp"
#include "xel_application.hpp"

#include <exception>
#include <iostream>
#include <string_view>

namespace
{
int run()
{
    xel::Application app{1200, 800, "XEL UI Demo"};
    auto demo = xel::demo::build(*app.uisys.root());

    bool f1_prev = false;

    while (!app.window_.should_close()) {
        glfwPollEvents();

        const bool f1_pressed = glfwGetKey(app.window_.get_glfw_window(), GLFW_KEY_F1) == GLFW_PRESS;
        if (f1_pressed && !f1_prev) demo.debug->toggle();
        f1_prev = f1_pressed;

        app.renderer_.begin();

        if (demo.debug->visible()) {
            demo.debug->update(app.renderer_.frame_stats(), app.uisys.hovered(), app.uisys.focused(),
                               demo.scroll->scroll_offset_vec());
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
