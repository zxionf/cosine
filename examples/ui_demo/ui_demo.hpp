#pragma once

#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/color_rect.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/root.hpp"
#include "ui/scroll_view.hpp"
#include "ui/slider.hpp"
#include "ui/text_input.hpp"
#include "ui/tooltip.hpp"
#include "ui/vbox.hpp"

#include <format>
#include <memory>
#include <string>

namespace xel::demo
{
struct UiDemo
{
    ui::ScrollView* scroll = nullptr;
    ui::Panel*      panel = nullptr;
    ui::Label*      status = nullptr;
    ui::Label*      value = nullptr;
    ui::Label*      input_status = nullptr;
    ui::Button*     button = nullptr;
    ui::Checkbox*   checkbox = nullptr;
    ui::Slider*     slider = nullptr;
    ui::TextInput*  input = nullptr;
    ui::Tooltip*    tooltip = nullptr;
};

inline UiDemo build(ui::UIRoot& root)
{
    UiDemo demo;

    auto scroll = std::make_unique<ui::ScrollView>();
    scroll->set_pos(0.0f, 0.0f);
    scroll->set_max_size({400.0f, 600.0f});

    auto content = std::make_unique<ui::Vbox>(0.0f, 0.0f, 400.0f, 600.0f);
    for (int i = 0; i < 20; ++i) {
        content->add(std::make_unique<ui::ColorRect>(
            0.0f, 0.0f, 400.0f, 100.0f,
            glm::vec4{0.0f, 1.0f - static_cast<float>(i) / 19.0f, 0.0f, 1.0f}));
    }
    scroll->set_content(std::move(content));
    demo.scroll = scroll.get();
    root.add(std::move(scroll));

    auto title = std::make_unique<ui::Label>("XEL", 144.0f, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    title->set_pos(700.0f, 160.0f);
    root.add(std::move(title));

    auto panel = std::make_unique<ui::Panel>(700.0f, 460.0f, 360.0f, 280.0f);
    panel->set_min_size({360.0f, 280.0f});
    panel->add(std::make_unique<ui::Label>("Controls"));

    auto status = std::make_unique<ui::Label>("clicks: 0");
    demo.status = status.get();
    panel->add(std::move(status));

    auto button = std::make_unique<ui::Button>("Click me", [status_ptr = demo.status]() {
        static int clicks = 0;
        ++clicks;
        status_ptr->set_text(std::format("clicks: {}", clicks));
    });
    demo.button = button.get();
    panel->add(std::move(button));

    auto checkbox = std::make_unique<ui::Checkbox>("Enable feature", true);
    demo.checkbox = checkbox.get();
    panel->add(std::move(checkbox));

    auto value = std::make_unique<ui::Label>("value: 0.00");
    demo.value = value.get();

    auto slider = std::make_unique<ui::Slider>(320.0f);
    demo.slider = slider.get();
    slider->set_on_change([value_ptr = demo.value](float v) {
        value_ptr->set_text(std::format("value: {:.2f}", v));
    });
    panel->add(std::move(slider));
    panel->add(std::move(value));

    auto input_status = std::make_unique<ui::Label>("input: ");
    demo.input_status = input_status.get();

    auto input = std::make_unique<ui::TextInput>("", 320.0f);
    demo.input = input.get();
    input->set_on_change([input_status_ptr = demo.input_status](const std::string& text) {
        input_status_ptr->set_text("input: " + text);
    });
    panel->add(std::move(input));
    panel->add(std::move(input_status));

    auto wrapped = std::make_unique<ui::Label>(
        "This is a long wrapped label used to verify automatic line wrapping and ellipsis in the UI.");
    wrapped->set_wrap(true);
    wrapped->set_max_lines(2);
    wrapped->set_ellipsis(true);
    panel->add(std::move(wrapped));

    panel->add(std::make_unique<ui::Label>("中文：你好，世界"));
    auto cjk_input = std::make_unique<ui::TextInput>("中文输入", 320.0f);
    panel->add(std::move(cjk_input));
    panel->add(std::make_unique<ui::Label>("动态字形：囧"));

    demo.panel = panel.get();
    root.add(std::move(panel));

    auto tooltip = std::make_unique<ui::Tooltip>("Overlay tooltip");
    demo.tooltip = tooltip.get();
    root.add_overlay(std::move(tooltip));
    demo.tooltip->show_at({720.0f, 200.0f});

    return demo;
}
} // namespace xel::demo
