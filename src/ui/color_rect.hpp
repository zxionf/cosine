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

    glm::vec2 measure(glm::vec2 constraints) override
    {
        return local_bounds().size;
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void on_mouse_enter() override
    {
        color_ = glm::vec4(46.f/255.f, 168.f/255.f, 1, 1.0f);
    }

    void on_mouse_leave() override
    {
        color_ = glm::vec4(184.f/255.f, 225.f/255.f, 1, 1.0f);
    }

    bool on_scroll(glm::vec2 pos, float delta) override
    {
        color_ = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return false;
    }
private:
    glm::vec4 color_;
};
} // namespace xel::ui