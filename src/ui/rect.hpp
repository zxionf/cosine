#pragma once

#include "widget.hpp"
#include "../graphics/rectangle.hpp"

namespace xel::ui
{
struct Rect
{
    glm::vec2 pos;
    glm::vec2 size;

    float left()   const { return pos.x; }
    float right()  const { return pos.x + size.x; }
    float top()    const { return pos.y; }
    float bottom() const { return pos.y + size.y; }
};

template <std::size_t I>
float get(const Rect& r) {
    if constexpr (I == 0) return r.pos.x;
    else if constexpr (I == 1) return r.pos.y;
    else if constexpr (I == 2) return r.size.x;
    else                  return r.size.y;
}
} // namespace xel::ui

namespace std {

template <>
struct tuple_size<xel::ui::Rect> : integral_constant<size_t, 4> {};

template <size_t I>
struct tuple_element<I, xel::ui::Rect> {
    using type = float;
};

} // namespace std