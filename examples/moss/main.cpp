#include "xel/version.hpp"
#include "xel_application.hpp"

#include "ui/containers/vbox.hpp"
#include "ui/containers/panel.hpp"
#include "ui/containers/scroll_view.hpp"
#include "ui/composites/window.hpp"
#include "ui/core/color_rect.hpp"

#include <iostream>
#include <memory>

int main()
{
    xel::Application app{600,400,"moss"};

    std::cout << "xel" << xel::version_string() << std::endl;

    xel::ui::Theme base_theme = xel::ui::Theme::dark();
    base_theme = base_theme.with_rounding(false)
        .with_radius_scale(0.0f)
        .with_focus_policy(xel::ui::FocusPolicy::Always)
        .with_transition_ms(60.0f);
    base_theme.scroll_mode = xel::ui::ScrollMode::Smooth;
    app.uisys.root()->set_theme(base_theme);
    app.set_ui_scale(2.0f);

    auto panel = std::make_unique<xel::ui::Panel>(20,20, 600,400);
    panel->set_min_size(glm::vec2{600,400});
    panel->set_main_align(xel::ui::MainAlign::Center);
    // auto vbox = std::make_unique<xel::ui::Vbox>(0,0, 200,200);
    // vbox->set_min_size(glm::vec2{300,200});
    auto color_rect = std::make_unique<xel::ui::ColorRect>(0,0, 200,100, glm::vec4{0,1,0,1});
    color_rect->set_align(xel::ui::Align::h_stretch());
    
    auto window = std::make_unique<xel::ui::Window>("xel", glm::vec2{30,30}, glm::vec2{300,200});
    auto scroll_view = std::make_unique<xel::ui::ScrollView>();
    // scroll_view->set_pos(20,20);
    // scroll_view->set_max_size(glm::vec2{600,100});
    auto color_rect2 = std::make_unique<xel::ui::ColorRect>(0,0, 200,400, glm::vec4{0,1,0,1});
    scroll_view->set_content(std::move(color_rect2));
    window->add(std::move(scroll_view));

    panel->add(std::move(color_rect));
    // scroll_view->set_content(std::move(panel));
    // panel->add(std::move(vbox));
    app.uisys.root()->add(std::move(panel));
    app.uisys.root()->add(std::move(window));
    app.uisys.root()->debug_recursive();

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