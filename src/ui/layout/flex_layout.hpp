#pragma once

#include "layout.hpp"

namespace xel::ui
{
    class FlexLayout : public Layout
    {
    public:
        LayoutConfig config;

        void calculatePositions(std::vector<Rect>& children, const Rect& parent) const override;
    };
}
