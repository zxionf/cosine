#include "panel.hpp"
#include "../ui_renderer.hpp"

namespace xel::ui
{
    Panel::Panel(const glm::vec2& position, const glm::vec2& size)
    {
        rect_ = Rect(position.x, position.y, size.x, size.y);
    }

    void Panel::render(UIRenderer& renderer)
    {
        if (!visible_) return;
        renderer.drawQuad({rect_.x, rect_.y}, {rect_.width, rect_.height}, bgColor_);
    }

    bool Panel::onEvent(const struct UIEvent& event)
    {
        (void)event;
        return false;
    }
}
