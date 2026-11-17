#include "xel/version.hpp"
#include "xel_application.hpp"

#include "ui/containers/vbox.hpp"
#include "ui/core/color_rect.hpp"

#include <iostream>
#include <memory>

int main()
{
    xel::Application app{600,400,"moss"};

    std::cout << "xel" << xel::version_string() << std::endl;

    auto vbox = std::make_unique<xel::ui::Vbox>(400, 400, 200, 400);
    vbox->debug();
    vbox->set_size(400,400);
    auto color_rect = std::make_unique<xel::ui::ColorRect>(0,0, 200,100, glm::vec4{0,1,0,1});

    vbox->adopt(std::move(color_rect));
    app.uisys.root()->add(std::move(vbox));

    while(!app.window_.should_close()) {
        glfwPollEvents();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        app.renderer_.begin();

        app.uisys.tick();

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();
    }

    app.vulkan_context_.device().waitIdle();
    return 0;
}