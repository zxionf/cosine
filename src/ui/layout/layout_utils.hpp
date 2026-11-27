#pragma once

#include "ui/geom/layout_metrics.hpp"
#include "ui/node/widget.hpp"

#include <algorithm>

namespace xel::ui::layout
{
// 内容区（扣掉 padding）：原先 Grid/Wrap/TabView/ScrollPanel/LinearLayout 各算一份（UI-36 收敛）。
inline Rect content_rect(const Widget& widget, Rect slot)
{
    const float pad_l = widget.padding_left();
    const float pad_t = widget.padding_top();
    return {
        {slot.pos.x + pad_l, slot.pos.y + pad_t},
        {std::max(0.0f, slot.size.x - pad_l - widget.padding_right()),
         std::max(0.0f, slot.size.y - pad_t - widget.padding_bottom())}
    };
}

// 摆放 Positioned（absolute）子项：相对内容区原点，尺寸走 measure_child 的统一规则。
inline void place_positioned_children(LayoutNode& parent, Rect content, glm::vec2 constraints)
{
    // UI-56：树存 Node，布局摆放需要布局属性 → 用 for_each_layout_child 的布局视图。
    parent.for_each_layout_child([&](LayoutNode& child) {
        if (child.display() != LayoutNode::Display::Positioned) return;
        const glm::vec2 size = LayoutNode::measure_child(child, constraints);
        child.arrange({{content.pos.x + child.absolute_pos().x,
                        content.pos.y + child.absolute_pos().y},
                       size});
    });
}
} // namespace xel::ui::layout
