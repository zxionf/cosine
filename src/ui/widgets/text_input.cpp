#include "text_input.hpp"
#include "ui_window.hpp"
#include "../ui_renderer.hpp"

namespace xel::ui
{
    TextInput::TextInput(const glm::vec2& position, const std::string& placeholder)
    : placeholder_(placeholder)
    {
        rect_ = Rect(position.x, position.y, 200, 30);
    }

    void TextInput::render(UIRenderer& renderer)
    {
        if (!visible_) return;
        glm::vec4 border(0.4f, 0.4f, 0.45f, 1.0f);
        glm::vec4 bg(focused_ ? 0.18f : 0.14f, focused_ ? 0.18f : 0.14f, focused_ ? 0.22f : 0.18f, 1.0f);
        renderer.drawQuad({rect_.x - 1, rect_.y - 1}, {rect_.width + 2, rect_.height + 2}, border);
        renderer.drawQuad({rect_.x, rect_.y}, {rect_.width, rect_.height}, bg);

        if (!text_.empty())
        {
            glm::vec4 textColor(0.9f, 0.9f, 0.9f, 1.0f);
            renderer.drawText({rect_.x + 6, rect_.y + 6}, text_, textColor);
        }
        else if (!focused_)
        {
            glm::vec4 placeholderColor(0.45f, 0.45f, 0.5f, 1.0f);
            renderer.drawText({rect_.x + 6, rect_.y + 6}, placeholder_, placeholderColor);
        }

        if (focused_)
        {
            float cursorX = rect_.x + 6 + text_.size() * 8.f;
            float t = cursorBlink_ - static_cast<int>(cursorBlink_);
            if (t < 0.5f)
            {
                glm::vec4 cursorColor(0.8f, 0.8f, 0.8f, 1.0f);
                renderer.drawQuad({cursorX, rect_.y + 4}, {1.5f, rect_.height - 8}, cursorColor);
            }
        }
    }

    bool TextInput::onEvent(const struct UIEvent& event)
    {
        if (!visible_ || !enabled_) return false;

        if (event.type == UIEvent::Type::MouseButton && event.mousePressed)
        {
            if (rect_.contains({event.mouseX, event.mouseY}))
            {
                focused_ = true;
                cursorBlink_ = 0.f;
                return true;
            }
            else
            {
                focused_ = false;
            }
        }

        if (event.type == UIEvent::Type::KeyPress && focused_)
        {
            if (event.key == 259 && !text_.empty())
            {
                text_.pop_back();
                cursorBlink_ = 0.f;
                return true;
            }
        }

        if (event.type == UIEvent::Type::Char && focused_)
        {
            if (event.character >= 32 && event.character <= 126)
            {
                text_ += event.character;
                cursorBlink_ = 0.f;
                return true;
            }
        }

        return false;
    }

    void TextInput::update(float dt)
    {
        if (focused_)
            cursorBlink_ += dt;
    }
}
