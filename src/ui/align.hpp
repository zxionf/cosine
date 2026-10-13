#pragma once

namespace xel::ui
{
struct Alignment {
    float x = 0.f;   // 0=左, 0.5=中, 1=右
    float y = 0.f;   // 0=上, 0.5=中, 1=下

    static constexpr Alignment top_left()     { return {0.0f, 0.0f}; }
    static constexpr Alignment top_center()   { return {0.5f, 0.0f}; }
    static constexpr Alignment top_right()    { return {1.0f, 0.0f}; }
    static constexpr Alignment center_left()  { return {0.0f, 0.5f}; }
    static constexpr Alignment center()       { return {0.5f, 0.5f}; }
    static constexpr Alignment center_right() { return {1.0f, 0.5f}; }
    static constexpr Alignment bottom_left()  { return {0.0f, 1.0f}; }
    static constexpr Alignment bottom_center(){ return {0.5f, 1.0f}; }
    static constexpr Alignment bottom_right() { return {1.0f, 1.0f}; }
};
} // namespace xel::ui
