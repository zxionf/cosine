#include "ui/button.hpp"
#include "ui/color_rect.hpp"
#include "ui/list.hpp"
#include "ui/splitter.hpp"
#include "ui/text_input.hpp"
#include "ui/tooltip.hpp"
#include "ui/checkbox.hpp"
#include "ui/root.hpp"
#include "ui/slider.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{

xel::event::InputEvent mouse(xel::event::InputEvent::Type type, glm::vec2 pos)
{
    xel::event::InputEvent event;
    event.type = type;
    event.mouse_pos = pos;
    return event;
}

xel::event::InputEvent key(xel::event::KeyCode code, xel::event::Modifier mods = xel::event::Modifier::None)
{
    xel::event::InputEvent event;
    event.type = xel::event::InputEvent::Type::KeyDown;
    event.key = code;
    event.mods = mods;
    return event;
}

xel::event::InputEvent char_input(char32_t codepoint)
{
    xel::event::InputEvent event;
    event.type = xel::event::InputEvent::Type::CharInput;
    event.codepoint = codepoint;
    return event;
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);

    auto button = std::make_unique<xel::ui::Button>("OK");
    auto* button_ptr = button.get();
    int clicks = 0;
    button_ptr->set_on_click([&clicks] { ++clicks; });
    button_ptr->set_global_bounds({{0.0f, 0.0f}, {100.0f, 40.0f}});
    button_ptr->set_pos(0.0f, 0.0f);
    button_ptr->set_size(100.0f, 40.0f);
    root->add(std::move(button));

    auto checkbox = std::make_unique<xel::ui::Checkbox>("Feature", false);
    auto* checkbox_ptr = checkbox.get();
    bool checked = false;
    checkbox_ptr->set_on_change([&checked](bool value) { checked = value; });
    checkbox_ptr->set_global_bounds({{0.0f, 100.0f}, {200.0f, 40.0f}});
    checkbox_ptr->set_pos(0.0f, 100.0f);
    checkbox_ptr->set_size(200.0f, 40.0f);
    root->add(std::move(checkbox));

    auto slider = std::make_unique<xel::ui::Slider>(200.0f);
    auto* slider_ptr = slider.get();
    float slider_value = -1.0f;
    slider_ptr->set_on_change([&slider_value](float value) { slider_value = value; });
    slider_ptr->set_global_bounds({{0.0f, 200.0f}, {200.0f, 40.0f}});
    slider_ptr->set_pos(0.0f, 200.0f);
    slider_ptr->set_size(200.0f, 40.0f);
    root->add(std::move(slider));

    auto text_input = std::make_unique<xel::ui::TextInput>("", 220.0f);
    auto* text_input_ptr = text_input.get();
    text_input_ptr->set_global_bounds({{0.0f, 300.0f}, {220.0f, 40.0f}});
    text_input_ptr->set_pos(0.0f, 300.0f);
    text_input_ptr->set_size(220.0f, 40.0f);
    root->add(std::move(text_input));
    xel::ui::UISystem ui{std::move(root)};

    // Button click
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {10.0f, 10.0f}));
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, {10.0f, 10.0f}));
    assert(clicks == 1);
    assert(ui.focused() == button_ptr);

    // Button keyboard activation
    ui.dispatch(key(xel::event::KeyCode::Enter));
    assert(clicks == 2);

    // Checkbox click
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {10.0f, 110.0f}));
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, {10.0f, 110.0f}));
    assert(checked);
    assert(checkbox_ptr->checked());

    // Slider drag
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {146.5f, 220.0f}));
    assert(slider_value > 0.70f && slider_value < 0.80f);
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseMove, {53.5f, 220.0f}));
    assert(slider_value > 0.20f && slider_value < 0.30f);
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, {53.5f, 220.0f}));

    // Slider keyboard
    const float before = slider_ptr->value();
    ui.dispatch(key(xel::event::KeyCode::Right));
    assert(slider_ptr->value() > before);

    // Tab：slider -> text input
    ui.dispatch(key(xel::event::KeyCode::Tab));
    assert(ui.focused() == text_input_ptr);

    ui.dispatch(char_input(U'A'));
    ui.dispatch(char_input(U'B'));
    assert(text_input_ptr->text() == "AB");
    ui.dispatch(key(xel::event::KeyCode::Backspace));
    assert(text_input_ptr->text() == "A");

    ui.dispatch(key(xel::event::KeyCode::A, xel::event::Modifier::Control));
    ui.dispatch(char_input(U'C'));
    assert(text_input_ptr->text() == "C");

    // Tab：text input -> button（循环）
    ui.dispatch(key(xel::event::KeyCode::Tab));
    assert(ui.focused() == button_ptr);

    // List
    xel::ui::List list{200.0f, 100.0f};
    list.add_item(std::make_unique<xel::ui::ColorRect>(0, 0, 200, 30, glm::vec4{1.0f}));
    list.add_item(std::make_unique<xel::ui::ColorRect>(0, 0, 200, 30, glm::vec4{1.0f}));
    assert(list.item_count() == 2);
    list.measure({200.0f, 100.0f});
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    assert(list.content_size().y > 0.0f);

    // Splitter
    xel::ui::Splitter splitter{200.0f, 100.0f};
    auto first = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    auto second = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    auto* first_ptr = first.get();
    auto* second_ptr = second.get();
    splitter.set_panes(std::move(first), std::move(second));
    splitter.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    assert(first_ptr->width() > 0.0f);
    assert(second_ptr->width() > 0.0f);
    assert(second_ptr->x() > first_ptr->x());
    splitter.set_ratio(0.25f);
    splitter.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    assert(first_ptr->width() < second_ptr->width());

    // Tooltip
    xel::ui::Tooltip tooltip("tip");
    assert(!tooltip.visible());
    tooltip.show_at({10.0f, 10.0f});
    assert(tooltip.visible());
    const glm::vec2 tip_size = tooltip.measure({100.0f, 100.0f});
    assert(tip_size.x > 0.0f && tip_size.y > 0.0f);
    tooltip.hide();
    assert(!tooltip.visible());

    std::cout << "ui control tests passed\n";
    return 0;
}
