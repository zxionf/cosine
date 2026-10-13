#pragma once

#include "../events/event.hpp"

#include <iostream>

namespace xel::ui
{
class Tester
{
public:
    void process_event(const event::InputEvent& event) {
        switch (event.type) {
        case event::InputEvent::Type::MouseMove:
            // std::cout << "Mouse move: " << event.mouse_pos_x << ", " << event.mouse_pos_y << "\r";
            break;
        case event::InputEvent::Type::MouseScroll:
            std::cout << "Mouse scroll: " << event.scroll_delta << "\r";
            break;
        default:
            break;
        }
    }
private:
};
}