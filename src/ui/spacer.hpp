#pragma once

#include "widget.hpp"

namespace xel::ui
{
class Spacer : public Widget
{
public:
    explicit Spacer(glm::vec2 size)
    {
        set_size(size);
    }

    Spacer(float width, float height)
    {
        set_size(width, height);
    }

    glm::vec2 measure(glm::vec2) override { return local_bounds().size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawBatch&) const override {}
    bool hit_testable() const override { return false; }
};
} // namespace xel::ui
