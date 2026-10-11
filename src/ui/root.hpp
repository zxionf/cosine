#pragma once

#include "widget.hpp"

#include <memory>

namespace xel::ui
{
    class UIRoot : public Widget
    {
    public:
        UIRoot(float x, float y, float width, float height);
        ~UIRoot() = default;

        void debug() override { debug_ = true; }
        void add(std::unique_ptr<Widget> child);
        void append_to(graphics::DrawBatch& batch) const override;
        void layout() override;

        float x() const override { return x_; }
        float y() const override { return y_; }
        float width() const override { return width_; }
        float height() const override { return height_; }

        void set_position(float x, float y) override { x_ = x; y_ = y; }
        void set_size(float width, float height) override { width_ = width; height_ = height; }
    private:
        float x_ = 0.f;
        float y_ = 0.f;
        float width_ = 0.f;
        float height_ = 0.f;

        float spacing_ = 0.f;
        float padding_ = 0.f;

        std::vector<std::unique_ptr<Widget>> children_;

        bool debug_ = false;
    };
} // namespace xel::ui