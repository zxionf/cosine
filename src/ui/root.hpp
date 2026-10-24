#pragma once

#include "widget.hpp"

#include <memory>

namespace xel::ui
{
// UIRoot 是自由画布：子控件保留自己的局部坐标，Root 只负责 global bounds 与裁剪边界。
class UIRoot : public Widget
{
public:
    UIRoot(float x, float y, float width, float height);
    ~UIRoot() = default;

    void add(std::unique_ptr<Widget> child) override;
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    bool hit_testable() const override { return false; }
    void append_to(graphics::DrawList& list) const override;
    void layout() override;
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;

private:
    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui
