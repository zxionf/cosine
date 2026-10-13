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
    void add(std::unique_ptr<Widget> child) override;
    void append_to(graphics::DrawBatch& batch) const override;
    void layout() override;

private:
    float spacing_ = 0.f;
    float padding_ = 0.f;

    std::vector<std::unique_ptr<Widget>> children_;

    bool debug_ = false;
};
} // namespace xel::ui