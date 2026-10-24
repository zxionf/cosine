#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <type_traits>

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

    bool contains(glm::vec2 p) const { return p.x >= left() && p.x <= right() && p.y >= top() && p.y <= bottom(); }

    Rect intersect(const Rect& other) const
    {
        const glm::vec2 lo{glm::max(pos, other.pos)};
        const glm::vec2 hi{glm::min(pos + size, other.pos + other.size)};
        return {lo, glm::max(hi - lo, glm::vec2{0.0f})};
    }
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
