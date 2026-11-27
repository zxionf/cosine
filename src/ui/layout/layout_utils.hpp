#pragma once

#include "ui/layout_metrics.hpp"
#include "ui/widget.hpp"

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
inline void place_positioned_children(Widget& parent, Rect content, glm::vec2 constraints)
{
    for (const auto& child : parent.children()) {
        if (child->display() != Widget::Display::Positioned) continue;
        const glm::vec2 size = Widget::measure_child(*child, constraints);
        child->arrange({{content.pos.x + child->absolute_pos().x,
                         content.pos.y + child->absolute_pos().y}, size});
    }
}
} // namespace xel::ui::layout
