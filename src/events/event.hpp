#pragma once

#include <glm/glm.hpp>

namespace xel::event
{
struct InputEvent
{
    enum class Type {
        MouseMove,
        MouseButtonDown,
        MouseButtonUp,
        MouseScroll,
        KeyDown,
        KeyUp,
        CharInput,
    };
    Type type = Type::MouseMove;

    // mouse
    glm::vec2 mouse_pos{};
    int   mouse_button = 0;
    float scroll_delta_x = 0.0f;
    float scroll_delta = 0.0f;

    // keyboard
    int      key = 0;
    int      mods = 0;
    char32_t codepoint = 0;

    bool handled = false;
    void consume() { handled = true; }
};
} // namespace xel::event
