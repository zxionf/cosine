#pragma once

#include "../graphics/line.hpp"
#include "widget.hpp"

#include <memory>

namespace xel::ui
{
    class Vbox : public Widget
    {
    public:
        Vbox(float x, float y, float width, float height);
        Vbox(glm::vec2 pos, glm::vec2 size);
        ~Vbox();
        void debug() override { debug_ = true; }

        void add(std::unique_ptr<Widget> child);
        void append_to(graphics::DrawBatch& batch) const override;

        void layout() override;

        float x() const override { return x_; }
        float y() const override { return y_; }
        float width() const override { return width_; }
        float height() const override { return height_; }

        // void set_position(float x, float y) override { x_ = x; y_ = y; }
        // void set_size(float width, float height) override { width_ = width; height_ = height; }
    private:

        float spacing_ = 4.0f;
        float padding_ = 4.0f;

        std::vector<std::unique_ptr<Widget>> children_;

        bool debug_ = false;
    };
} // namespace xel::ui