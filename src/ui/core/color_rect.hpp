#pragma once

#include "ui/widget.hpp"
#include "graphics/rectangle.hpp"

namespace xel::ui
{
class ColorRect : public Widget
{
public:
    ColorRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color)
    {
        set_pos(pos);
        set_size(size);
        set_preferred_size(size);
        color_ = color;
    }

    ColorRect(float x, float y, float width, float height, glm::vec4 color)
    {
        set_pos(x, y);
        set_size(width, height);
        set_preferred_size({width, height});
        color_ = color;
    }

    void set_color(glm::vec4 color) { color_ = color; }
    glm::vec4 color() const { return color_; }

    void append_to(graphics::DrawList& list) const override
    {
        graphics::Rectangle rectangle(x(), y(), width(), height(), color_);
        rectangle.append_to(list);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return clamp_size(preferred_size());
    }

    void arrange(Rect slot) override
    {
        set_geometry(slot);
    }

    void on_mouse_enter() override
    {
        color_ = glm::vec4(46.f/255.f, 168.f/255.f, 1, 1.0f);
    }

    void on_mouse_leave() override
    {
        color_ = glm::vec4(184.f/255.f, 225.f/255.f, 1, 1.0f);
    }

    bool on_scroll(glm::vec2 pos, glm::vec2 delta) override
    {
        (void)pos;
        (void)delta;
        color_ = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return false;
    }
private:
    glm::vec4 color_;
};
} // namespace xel::ui