#pragma once

#include "widget.hpp"
#include "root.hpp"
#include "../events/event.hpp"

namespace xel::ui
{
class UISystem
{
public:
    UISystem();
    UIRoot* root() { return root_.get();}
    void tick() {}

    void dispatch(const event::InputEvent& event);
private:
    Widget* deep_hit(Widget* widget, glm::vec2 pos);
    void dispatch_mouse_move(const event::InputEvent& event);

    std::unique_ptr<UIRoot> root_;

    Widget* hovered_ = nullptr;
    Widget* pressed_ = nullptr;
};
} // namespace xel::ui
