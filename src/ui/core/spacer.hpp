#pragma once

#include "ui/widget.hpp"

namespace xel::ui
{
class Spacer : public Widget
{
public:
    explicit Spacer(glm::vec2 size) : preferred_{size}
    {
        set_size(size);
    }

    Spacer(float width, float height) : Spacer(glm::vec2{width, height}) {}

    // 返回构造时的首选尺寸，而不是当前 arranged 尺寸：
    // 否则 flex_grow 分配过一次后，下一次 measure 会把放大后的尺寸当成自然尺寸，
    // 窗口缩小时无法再收缩。
    glm::vec2 measure(glm::vec2) override { return preferred_; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList&) const override {}
    bool hit_testable() const override { return false; }

private:
    glm::vec2 preferred_{0.0f, 0.0f};
};
} // namespace xel::ui
