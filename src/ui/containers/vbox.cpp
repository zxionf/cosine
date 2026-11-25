#include "ui/containers/vbox.hpp"

namespace xel::ui
{
Vbox::Vbox(float x, float y, float width, float height) : LinearLayout(Axis::Vertical)
{
    set_pos(x, y);
    set_size(width, height);
    set_axis_defaults(Align::top_center());
}

Vbox::Vbox(glm::vec2 pos, glm::vec2 size) : LinearLayout(Axis::Vertical)
{
    set_pos(pos);
    set_size(size);
    set_axis_defaults(Align::top_center());
}
} // namespace xel::ui
