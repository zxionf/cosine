#pragma once

#include "ui/containers/linear_layout.hpp"

namespace xel::ui
{
// 垂直流：主轴 y、交叉轴 x（UI-36 起是 LinearLayout 的薄封装）。
class Vbox : public LinearLayout
{
public:
    Vbox(float x, float y, float width, float height);
    Vbox(glm::vec2 pos, glm::vec2 size);
};
} // namespace xel::ui
