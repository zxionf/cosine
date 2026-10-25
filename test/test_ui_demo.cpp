#include "ui_demo.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
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

glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.global_bounds();
    return {bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f};
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1200.0f, 800.0f);
    auto demo = xel::demo::build(*root);

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(ui.root()->overlays().size() == 2);
    assert(demo.debug != nullptr);
    assert(!demo.debug->visible());
    assert(demo.tooltip->visible());
    assert(demo.tooltip->global_bounds().size.x > 0.0f);
    assert(demo.scroll->content_size().y > demo.scroll->viewport_size().y);

    auto scroll_event = mouse(xel::event::InputEvent::Type::MouseScroll, {200.0f, 300.0f});
    scroll_event.scroll_delta = -1.0f;
    ui.dispatch(scroll_event);
    assert(demo.scroll->scroll_offset() > 0.0f);

    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, center(*demo.button)));
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, center(*demo.button)));
    assert(demo.status->text() == "clicks: 1");

    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, center(*demo.input)));
    ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, center(*demo.input)));

    xel::event::InputEvent chars;
    chars.type = xel::event::InputEvent::Type::CharInput;
    chars.codepoint = U'A';
    ui.dispatch(chars);
    assert(demo.input->text() == "A");

    std::cout << "ui demo tests passed\n";
    return 0;
}
