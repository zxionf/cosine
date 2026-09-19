#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace xel::ui
{
    struct Rect
    {
        float x = 0, y = 0, width = 0, height = 0;
        Rect() = default;
        Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

        glm::vec2 center() const { return {x + width / 2.f, y + height / 2.f}; }
        glm::vec2 min() const { return {x, y}; }
        glm::vec2 max() const { return {x + width, y + height}; }
        bool contains(const glm::vec2& p) const
        {
            return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
        }
    };

    enum class Direction : uint8_t
    {
        Horizontal,
        Vertical
    };

    struct LayoutConfig
    {
        Direction direction = Direction::Vertical;
        float spacing = 4.0f;
        float padding = 8.0f;
        glm::vec2 alignment = {0.f, 0.f};
        bool wrap = false;
    };

    class Layout
    {
    public:
        virtual ~Layout() = default;
        virtual void calculatePositions(std::vector<Rect>& children, const Rect& parent) const = 0;
    };
}
