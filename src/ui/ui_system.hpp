#pragma once

#include "root.hpp"
#include "../events/event.hpp"
#include "../backend/window.hpp"

namespace xel::ui
{
class UISystem
{
public:
    UISystem(backend::Window& window);
    UIRoot* root() { return root_.get();}
    void tick();

    void dispatch(const event::InputEvent& event);
private:
    Widget* deep_hit(Widget* widget, glm::vec2 pos);
    void dispatch_mouse_move(const event::InputEvent& event);
    void dispatch_mouse_button(const event::InputEvent& event);
    void dispatch_mouse_scroll(const event::InputEvent& event);

    std::unique_ptr<UIRoot> root_;

    Widget* hovered_ = nullptr;
    Widget* pressed_ = nullptr;

    uint32_t last_width_ = 0;
    uint32_t last_height_ = 0;

    backend::Window& window_;
};
} // namespace xel::ui
