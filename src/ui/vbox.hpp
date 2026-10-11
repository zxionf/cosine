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
        ~Vbox();
        void debug() { debug_ = true; }

        void add(std::unique_ptr<Widget> child);
        void append_to(graphics::DrawBatch& batch) const override;

        void layout();

        float x() const override { return x_; }
        float y() const override { return y_; }
        float width() const override { return width_; }
        float height() const override { return height_; }

        void set_position(float x, float y) override { x_ = x; y_ = y; }
    private:
        float x_ = 0;
        float y_ = 0;
        float width_ = 0;
        float height_ = 0;

        float spacing_ = 4.0f;
        float padding_ = 4.0f;

        std::vector<std::unique_ptr<Widget>> children_;

        bool debug_ = false;
    };
} // namespace xel::ui