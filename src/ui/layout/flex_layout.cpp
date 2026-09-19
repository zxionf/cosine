#include "flex_layout.hpp"

namespace xel::ui
{
    void FlexLayout::calculatePositions(std::vector<Rect>& children, const Rect& parent) const
    {
        float x = parent.x + config.padding;
        float y = parent.y + config.padding;
        float rowHeight = 0.f;
        float rowWidth = 0.f;

        for (size_t i = 0; i < children.size(); ++i)
        {
            if (config.direction == Direction::Horizontal)
            {
                children[i].x = x;
                children[i].y = y;
                x += children[i].width + config.spacing;
                rowHeight = std::max(rowHeight, children[i].height);
                rowWidth += children[i].width + config.spacing;
            }
            else
            {
                children[i].x = x;
                children[i].y = y;
                y += children[i].height + config.spacing;
                rowWidth = std::max(rowWidth, children[i].width);
            }
        }
    }
}
