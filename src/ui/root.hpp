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

    void add(std::unique_ptr<Widget> child) override;
    void append_to(graphics::DrawBatch& batch) const override;
    void layout() override;
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;

private:
    float spacing_ = 0.f;
    float padding_ = 0.f;

    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui