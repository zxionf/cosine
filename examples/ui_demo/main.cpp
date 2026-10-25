#include "ui_demo.hpp"
#include "xel_application.hpp"

int main()
{
    xel::Application app{1200, 800, "XEL UI Demo"};
    xel::demo::build(*app.uisys.root());

    while (!app.window_.should_close()) {
        glfwPollEvents();
        app.uisys.tick();
        app.renderer_.begin();
        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();
    }

    app.vulkan_context_.device().waitIdle();
    return 0;
}
