#pragma once

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
        Type type;

        // mouse
        float mouse_pos_x  = 0.0f;
        float mouse_pos_y  = 0.0f;
        int   mouse_button = 0;
        float scroll_delta = 0.0f;

        // keyboard
        int  key        = 0;
        char char_input = 0;
    };

    struct Event
    {
        enum class Type {
            MouseEnter,
            MouseLeave,
            MouseDown,
            MouseUp,
            Click,
            Scroll,
            KeyDown,
            TextInput,
        };
        Type type;
        // Drawable* target = nullptr;   // 事件目标
        // glm::vec2 local_pos{0.0f};    // 相对控件左上角的坐标
        // float scroll_delta = 0.0f;
        // char char_input = 0;
    };
} // namespace xel::event
