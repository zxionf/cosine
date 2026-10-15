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

    void add(std::unique_ptr<Widget> child) override;
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    void append_to(graphics::DrawBatch& batch) const override;

    void layout() override;
    bool hit_testable() const override { return false; }
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;
    void set_cross_align(Align a) { cross_align_ = a; }
    Align resolve_align(const Widget& child) const;

private:

    float spacing_ = 4.0f;
    float padding_ = 4.0f;
    Align cross_align_ = Align::bottom_center();

    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui