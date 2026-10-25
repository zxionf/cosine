#pragma once

#include <cstdint>

namespace xel::event
{
// UI 层稳定键码；应用层负责把 GLFW/平台键码映射到这里。
enum class KeyCode : int
{
    Unknown = 0,
    Tab,
    Enter,
    KeypadEnter,
    Space,
    Backspace,
    Delete,
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    Escape,
    A,
};

enum class Modifier : std::uint32_t
{
    None    = 0,
    Shift   = 1u << 0,
    Control = 1u << 1,
    Alt     = 1u << 2,
    Super   = 1u << 3,
};

inline Modifier operator|(Modifier a, Modifier b)
{
    return static_cast<Modifier>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

inline bool has_modifier(Modifier mods, Modifier bit)
{
    return (static_cast<std::uint32_t>(mods) & static_cast<std::uint32_t>(bit)) != 0;
}
} // namespace xel::event
