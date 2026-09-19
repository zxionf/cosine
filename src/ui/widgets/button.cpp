#include "button.hpp"
#include "../ui_renderer.hpp"
#include "../ui_window.hpp"

namespace xel::ui
{
    Button::Button(const std::string& label, const Rect& rect, Callback callback)
    : label_(label), callback_(std::move(callback))
    {
        rect_ = rect;
    }

    void Button::render(UIRenderer& renderer)
    {
        if (!visible_) return;

        glm::vec4 bgNormal(0.25f, 0.55f, 0.85f, 1.0f);
        glm::vec4 bgHover(0.35f, 0.65f, 0.95f, 1.0f);
        glm::vec4 bgActive(0.2f, 0.45f, 0.75f, 1.0f);

        glm::vec4 bg = bgNormal;
        if (state_ == WidgetState::Hovered) bg = bgHover;
        if (state_ == WidgetState::Active) bg = bgActive;

        renderer.drawQuad({rect_.x, rect_.y}, {rect_.width, rect_.height}, bg);

        if (!label_.empty())
        {
            glm::vec4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
            float textWidth = label_.size() * 8.f;
            float textX = rect_.x + (rect_.width - textWidth) / 2.f;
            float textY = rect_.y + (rect_.height - 12.f) / 2.f;
            renderer.drawText({textX, textY}, label_, textColor);
        }
    }

    bool Button::onEvent(const struct UIEvent& event)
    {
        if (!visible_ || !enabled_) return false;

        if (event.type == UIEvent::Type::MouseMove)
        {
            if (rect_.contains({event.mouseX, event.mouseY}))
            {
                if (state_ != WidgetState::Active)
                    state_ = WidgetState::Hovered;
            }
            else
            {
                state_ = WidgetState::Normal;
            }
            return false;
        }

        if (event.type == UIEvent::Type::MouseButton)
        {
            if (event.mousePressed && rect_.contains({event.mouseX, event.mouseY}))
            {
                state_ = WidgetState::Active;
                return true;
            }
            if (!event.mousePressed && state_ == WidgetState::Active)
            {
                if (rect_.contains({event.mouseX, event.mouseY}))
                {
                    state_ = WidgetState::Hovered;
                    if (callback_) callback_();
                }
                else
                {
                    state_ = WidgetState::Normal;
                }
                return true;
            }
        }

        return false;
    }

    void Button::update(float dt)
    {
        (void)dt;
    }
}
