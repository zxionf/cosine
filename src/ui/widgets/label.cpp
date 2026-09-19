#include "label.hpp"
#include "../ui_renderer.hpp"
#include "../ui_window.hpp"

namespace xel::ui
{
    Label::Label(const std::string& text, const glm::vec2& position, float fontSize)
    : text_(text), fontSize_(fontSize)
    {
        float textWidth = text.size() * fontSize * 0.6f;
        rect_ = Rect(position.x, position.y, textWidth, fontSize + 4);
    }

    void Label::render(UIRenderer& renderer)
    {
        if (!visible_) return;
        glm::vec4 textColor(0.85f, 0.85f, 0.9f, 1.0f);
        renderer.drawText({rect_.x, rect_.y}, text_, textColor, fontSize_);
    }

    bool Label::onEvent(const struct UIEvent& event)
    {
        (void)event;
        return false;
    }

    void Label::setText(const std::string& text)
    {
        text_ = text;
        float textWidth = text.size() * fontSize_ * 0.6f;
        rect_.width = textWidth;
    }
}
