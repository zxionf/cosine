#pragma once

#include "ui/containers/linear_layout.hpp"

namespace xel::ui
{
// 水平流：主轴 x、交叉轴 y（UI-36 起是 LinearLayout 的薄封装）。
class Hbox : public LinearLayout
{
public:
    Hbox(float x, float y, float width, float height);
    Hbox(glm::vec2 pos, glm::vec2 size);
};
} // namespace xel::ui
