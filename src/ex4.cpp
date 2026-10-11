#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/ui_renderer.hpp"
#include "xel_application.hpp"
#include "ui/vbox.hpp"
#include "ui/rect.hpp"
#include "ui/root.hpp"

float srgb_to_linear(float c) {
    return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

glm::vec4 srgb_to_linear(glm::vec4 c) {
    return {srgb_to_linear(c.r), srgb_to_linear(c.g), srgb_to_linear(c.b), c.a};
}

const std::string text = R"(
and the universe said
everything you need is within you
you are stronger than you know
you are the daylight
you are the night
the darkness you fight is within you
the light you seek is within you
you are not alone
you are not separate from every
other thing
)";


int main()
{
    xel::Application app{1200, 800, "XEL"};

    xel::ui::Vbox vbox{800, 80, 200, 400};
    vbox.debug();
    vbox.add(std::make_unique<xel::ui::Rect>(0, 0, 100, 100, glm::vec4{0.1f, 0.5f, 0.0f, 1.0f}));
    vbox.add(std::make_unique<xel::ui::Rect>(0, 0, 100, 200, glm::vec4{0.1f, 0.5f, 0.4f, 1.0f}));
    vbox.layout();

    xel::ui::UIRoot root{0, 0, 1000, 600};

    while (!app.window_.should_close())
    {
        glfwPollEvents();
        app.renderer_.begin();
        float window_w = static_cast<float>(app.window_.get_extent().width);
        float window_h = static_cast<float>(app.window_.get_extent().height);
        double mx, my;
        glfwGetCursorPos(app.window_.get_glfw_window(), &mx, &my);
        float mouse_x = static_cast<float>(mx);
        float mouse_y = static_cast<float>(my);
        app.renderer_.draw_rect(0.0f, window_h-40.0f, window_w, 40.0f, {46.f/255.f, 168.f/255.f, 1, 1.0f});//#2ea8ff
        app.renderer_.draw_rect(0.0f, 0.0f, window_w, 40.0f, {46.f/255.f, 168.f/255.f, 1, 1.0f});//#2ea8ff

        app.renderer_.draw_rect(40.0f, 80.0f, 600.0f, window_h-160.0f, {184.f/255.f, 225.f/255.f, 1, 1.0f});//#b8e1ff
        app.renderer_.draw_text(60.0f, 100.0f, 24.0f, "XEL", {1.f, 0.f, 0.f, 1.0f});
        app.renderer_.draw_text(60.0f, 124.0f, 24.0f, text.c_str(), {0.f, 122.f/255.f, 209.f/255.f, 1.0f});//#007ad1

        // app.renderer_.set_scissor(700.0f, 160.0f, 432.0f, 108.0f);
        app.renderer_.set_scissor(mouse_x - 54.f, mouse_y - 54.f, 108.f, 108.0f);
        app.renderer_.draw_text(700.0f, 160.0f, 24.0f*6, "XEL", {1.f, 0.f, 0.f, 1.0f});
        app.renderer_.reset_scissor();

        app.renderer_.draw(vbox);

        uint32_t vertices_count = app.renderer_.vertices_count();
        uint32_t indices_count = app.renderer_.indices_count();
        app.renderer_.draw_text(23.0f, 23.0f, 24.0f, std::format("vertices: >{}", vertices_count), {0.0f, 0.0f, 0.0f, 0.8f});
        app.renderer_.draw_text(23.0f, 47.0f, 24.0f, std::format("indices: >{}", indices_count), {0.0f, 0.0f, 0.0f, 0.8f});
        app.renderer_.draw_text(20.0f, 20.0f, 24.0f, std::format("vertices: >{}", vertices_count), {0.0f, 1.0f, 0.0f, 0.8f});
        app.renderer_.draw_text(20.0f, 44.0f, 24.0f, std::format("indices: >{}", indices_count), {0.0f, 1.0f, 0.0f, 0.8f});
        app.renderer_.end();
    }
    app.vulkan_context_.device().waitIdle();
}