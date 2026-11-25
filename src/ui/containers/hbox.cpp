#include "ui/containers/hbox.hpp"

namespace xel::ui
{
Hbox::Hbox(float x, float y, float width, float height) : LinearLayout(Axis::Horizontal)
{
    set_pos(x, y);
    set_size(width, height);
    set_axis_defaults(Align::center_left());
}

Hbox::Hbox(glm::vec2 pos, glm::vec2 size) : LinearLayout(Axis::Horizontal)
{
    set_pos(pos);
    set_size(size);
    set_axis_defaults(Align::center_left());
}
} // namespace xel::ui
