// 字体实验室（EX-6）：左边属性、中间字符串预览、文字调试绘制与测量。
// 与产品代码解耦；只用于观察 FontAtlas 的度量与调试几何。

#include "graphics/font_atlas.hpp"
#include "graphics/text_debug.hpp"
#include "graphics/utf8.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/containers/panel.hpp"
#include "ui/core/drag_float.hpp"
#include "ui/core/label.hpp"
#include "ui/core/text_input.hpp"
#include "ui/root.hpp"
#include "ui/theme.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

namespace
{
constexpr float kW = 1200.0f;
constexpr float kH = 800.0f;

struct Lab
{
    xel::graphics::FontAtlas* atlas = nullptr;
    xel::ui::Label* preview = nullptr;
    xel::ui::ScrollPanel* scroll = nullptr;
    xel::ui::TextInput* text_input = nullptr;
    xel::ui::TextInput* char_input = nullptr;
    xel::ui::DragFloat* size_input = nullptr;
    xel::ui::Checkbox* glyph_boxes = nullptr;
    xel::ui::Checkbox* line_box = nullptr;
    xel::ui::Checkbox* baseline = nullptr;
    xel::ui::Checkbox* advance = nullptr;
    xel::ui::Checkbox* measure = nullptr;
    xel::ui::Label* string_info = nullptr;
    xel::ui::Label* char_info = nullptr;
    xel::ui::Label* glyph_info = nullptr;
    xel::ui::Label* metrics_info = nullptr;
    float text_size = 64.0f;
    std::string text = "Hello cosine";
    std::string ch = "A";
};

void update_debug(Lab& lab)
{
    xel::graphics::TextDebugOptions options;
    options.glyph_boxes = lab.glyph_boxes != nullptr && lab.glyph_boxes->checked();
    options.line_box = lab.line_box != nullptr && lab.line_box->checked();
    options.baseline = lab.baseline != nullptr && lab.baseline->checked();
    options.advance = lab.advance != nullptr && lab.advance->checked();
    if (lab.preview != nullptr) lab.preview->set_text_debug(options);
}

void update_measures(Lab& lab)
{
    if (lab.atlas == nullptr) return;

    const float size = lab.text_size;
    const auto measured = lab.atlas->measure(lab.text, size);
    int lines = 1;
    for (const char c : lab.text) {
        if (c == '\n') ++lines;
    }

    std::size_t index = 0;
    char32_t codepoint = 0;
    if (!lab.ch.empty()) codepoint = xel::graphics::utf8_decode(lab.ch, index);
    const auto* glyph = lab.atlas->glyph(codepoint, lab.atlas->glyph_pixel_size(size));
    const float advance = (glyph != nullptr ? glyph->advance_ratio : 0.5f) * size;
    const float bearing_x = (glyph != nullptr ? glyph->bearing_x_ratio : 0.0f) * size;
    const float bearing_y = (glyph != nullptr ? glyph->bearing_y_ratio : 0.0f) * size;
    const float glyph_w = (glyph != nullptr ? glyph->width_ratio : 0.0f) * size;
    const float glyph_h = (glyph != nullptr ? glyph->height_ratio : 0.0f) * size;

    const auto& metrics = lab.atlas->metrics();
    const float ascent = metrics.ascent_ratio * size;
    const float descent = metrics.descent_ratio * size;
    const float line_height = metrics.line_height_ratio * size;

    const bool show = lab.measure != nullptr && lab.measure->checked();
    if (lab.string_info != nullptr) {
        lab.string_info->set_visible(show);
        lab.string_info->set_text(
            std::format("string  {:.1f} x {:.1f}   lines {}", measured.x, measured.y, lines));
    }
    if (lab.char_info != nullptr) {
        lab.char_info->set_visible(show);
        lab.char_info->set_text(
            std::format("char    adv {:.1f}   box {:.1f}x{:.1f}", advance, glyph_w, glyph_h));
    }
    if (lab.glyph_info != nullptr) {
        lab.glyph_info->set_visible(show);
        lab.glyph_info->set_text(
            std::format("glyph   bearing {:.1f},{:.1f}", bearing_x, bearing_y));
    }
    if (lab.metrics_info != nullptr) {
        lab.metrics_info->set_visible(show);
        lab.metrics_info->set_text(std::format(
            "ascent {:.1f}   descent {:.1f}   line {:.1f}", ascent, descent, line_height));
    }

    if (lab.scroll != nullptr) lab.scroll->invalidate_measure();
}

void build(Lab& lab, xel::ui::UIRoot& root)
{
    auto left = std::make_unique<xel::ui::Panel>(12.0f, 12.0f, 320.0f, kH - 24.0f);
    left->set_min_size({320.0f, kH - 24.0f});
    left->set_max_size({320.0f, kH - 24.0f});
    left->set_padding(12.0f);
    left->set_spacing(8.0f);
    left->set_cross_align(xel::ui::Align::top_left());

    left->add(std::make_unique<xel::ui::Label>("Font Lab", 22.0f));

    left->add(std::make_unique<xel::ui::Label>("Text size (8 - 256)", 16.0f));
    auto size = std::make_unique<xel::ui::DragFloat>("", 120.0f);
    size->set_range(8.0f, 256.0f);
    size->set_value(lab.text_size);
    size->set_step(0.1f);
    size->set_on_change([&lab](float value) {
        lab.text_size = value;
        if (lab.preview != nullptr) lab.preview->set_text_size(value);
        update_measures(lab);
    });
    lab.size_input = size.get();
    left->add(std::move(size));

    left->add(std::make_unique<xel::ui::Label>("String", 16.0f));
    auto input = std::make_unique<xel::ui::TextInput>(lab.text, 260.0f);
    input->set_on_change([&lab](const std::string& text) {
        lab.text = text;
        if (lab.preview != nullptr) lab.preview->set_text(text);
        update_measures(lab);
    });
    lab.text_input = input.get();
    left->add(std::move(input));

    left->add(std::make_unique<xel::ui::Label>("Text debug", 16.0f));
    auto add_check = [&left, &lab](const char* name, bool initial,
                                   xel::ui::Checkbox*& slot) {
        auto box = std::make_unique<xel::ui::Checkbox>(name, initial, [&lab](bool) {
            update_debug(lab);
        });
        slot = box.get();
        left->add(std::move(box));
    };
    add_check("Glyph boxes", false, lab.glyph_boxes);
    add_check("Line box", false, lab.line_box);
    add_check("Baseline", false, lab.baseline);
    add_check("Advance", false, lab.advance);

    auto measure = std::make_unique<xel::ui::Checkbox>("Measure", true, [&lab](bool) {
        update_measures(lab);
    });
    lab.measure = measure.get();
    left->add(std::move(measure));

    left->add(std::make_unique<xel::ui::Label>("Char", 16.0f));
    auto char_input = std::make_unique<xel::ui::TextInput>(lab.ch, 80.0f);
    char_input->set_on_change([&lab](const std::string& text) {
        lab.ch = text;
        update_measures(lab);
    });
    lab.char_input = char_input.get();
    left->add(std::move(char_input));

    auto string_info = std::make_unique<xel::ui::Label>("string", 15.0f);
    lab.string_info = string_info.get();
    left->add(std::move(string_info));
    auto char_info = std::make_unique<xel::ui::Label>("char", 15.0f);
    lab.char_info = char_info.get();
    left->add(std::move(char_info));
    auto glyph_info = std::make_unique<xel::ui::Label>("glyph", 15.0f);
    lab.glyph_info = glyph_info.get();
    left->add(std::move(glyph_info));
    auto metrics_info = std::make_unique<xel::ui::Label>("metrics", 15.0f);
    lab.metrics_info = metrics_info.get();
    left->add(std::move(metrics_info));

    root.add(std::move(left));

    auto scroll = std::make_unique<xel::ui::ScrollPanel>(344.0f, 12.0f, kW - 356.0f, kH - 24.0f);
    scroll->set_min_size({kW - 356.0f, kH - 24.0f});
    scroll->set_max_size({kW - 356.0f, kH - 24.0f});
    scroll->set_padding(0.0f);
    scroll->scroll_view()->set_axis(xel::ui::ScrollView::Axis::Both);
    scroll->content()->set_padding(16.0f);
    scroll->content()->set_spacing(0.0f);
    scroll->content()->set_cross_align(xel::ui::Align::top_left());

    auto preview = std::make_unique<xel::ui::Label>(lab.text, lab.text_size);
    lab.preview = preview.get();
    scroll->add(std::move(preview));
    lab.scroll = scroll.get();
    root.add(std::move(scroll));
}

int run()
{
    xel::Application app{static_cast<int>(kW), static_cast<int>(kH), "font_lab"};

    Lab lab;
    build(lab, *app.uisys.root());
    lab.atlas = app.uisys.context()->font_atlas;
    update_debug(lab);
    update_measures(lab);

    if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
        app.uisys.tick(1.0f / 60.0f);

        auto* root = app.uisys.root();
        root->set_size(kW, kH);
        root->set_global_bounds({{0.0f, 0.0f}, {kW, kH}});
        root->mark_layout_dirty();
        root->layout();
        root->clear_layout_dirty();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        const auto pixels = app.renderer_.render_offscreen(*root, static_cast<uint32_t>(kW),
                                                          static_cast<uint32_t>(kH));
        const bool ok = xel::backend::vulkan::Renderer::write_png(
            png, static_cast<uint32_t>(kW), static_cast<uint32_t>(kH), pixels);
        std::cout << (ok ? "exported " : "export failed ") << png << '\n';
        return ok ? 0 : 1;
    }

    const char* exit_after_env = std::getenv("COSINE_EXIT_AFTER");
    const float exit_after = exit_after_env != nullptr ? std::atof(exit_after_env) : 0.0f;
    float total_time = 0.0f;

    while (!app.window_.should_close()) {
        glfwPollEvents();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        app.renderer_.begin();

        const auto& stats = app.renderer_.frame_stats();
        const float dt = std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);
        app.uisys.tick(dt);

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();

        total_time += dt;
        if (exit_after > 0.0f && total_time >= exit_after) {
            glfwSetWindowShouldClose(app.window_.get_glfw_window(), GLFW_TRUE);
        }
    }
    return 0;
}
} // namespace

int main()
{
    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
