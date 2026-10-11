#pragma once

#include "widget.hpp"
#include "../graphics/rectangle.hpp"

namespace xel::ui
{
    class Rect : public Widget
    {
    public:
        Rect(float x, float y, float width, float height, glm::vec4 color) : x_{x}, y_{y}, width_{width}, height_{height}, color_{color} {}
        void append_to(graphics::DrawBatch& batch) const override {
            graphics::Rectangle rectangle(x_, y_, width_, height_, color_);
            rectangle.append_to(batch);
        }
        float x() const override { return x_; }
        float y() const override { return y_; }
        float width() const override { return width_; }
        float height() const override { return height_; }

        void set_position(float x, float y) override { x_ = x; y_ = y; }
    private:
        float x_, y_, width_, height_;
        glm::vec4 color_;
    };
} // namespace xel::ui