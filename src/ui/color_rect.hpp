#pragma once

#include "widget.hpp"
#include "../graphics/rectangle.hpp"

namespace xel::ui
{
class ColorRect : public Widget
{
public:
    ColorRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color)
    {
        set_pos(pos);
        set_size(size);
        color_ = color;
    }

    ColorRect(float x, float y, float width, float height, glm::vec4 color)
    {
        set_pos(x, y);
        set_size(width, height);
        color_ = color;
    }

    void append_to(graphics::DrawBatch& batch) const override
    {
        graphics::Rectangle rectangle(x(), y(), width(), height(), color_);
        rectangle.append_to(batch);
    }
private:
    glm::vec4 color_;
};
} // namespace xel::ui