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
    ~Vbox() override;

    void add(std::unique_ptr<Widget> child) override;
    void clear_children() { children_.clear(); mark_layout_dirty(); }
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    void append_to(graphics::DrawList& list) const override;

    void layout() override;
    bool hit_testable() const override { return false; }
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;
    void set_cross_align(Align a) { cross_align_ = a; mark_layout_dirty(); }
    void set_spacing(float spacing) { spacing_ = spacing; mark_layout_dirty(); }
    void set_main_align(MainAlign a) { main_align_ = a; mark_layout_dirty(); }
    Align resolve_align(const Widget& child) const;
    void invalidate_measure() override { measure_valid_ = false; }

private:

    float spacing_ = 4.0f;
    Align cross_align_ = Align::top_center();
    MainAlign main_align_ = MainAlign::Start;

    mutable glm::vec2 cached_measure_{0.0f, 0.0f};
    mutable glm::vec2 cached_constraints_{-1.0f, -1.0f};
    mutable bool measure_valid_ = false;

    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui