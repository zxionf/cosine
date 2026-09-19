#include "slider.hpp"
#include "../ui_renderer.hpp"
#include "../ui_window.hpp"
#include <algorithm>

namespace xel::ui
{
    Slider::Slider(const glm::vec2& position, float minVal, float maxVal, float defaultValue)
    : minValue_(minVal), maxValue_(maxVal), value_(defaultValue)
    {
        rect_ = Rect(position.x, position.y, 200, 20);
    }

    void Slider::render(UIRenderer& renderer)
    {
        if (!visible_) return;
        glm::vec4 bg(0.3f, 0.3f, 0.35f, 1.0f);
        glm::vec4 fill(0.2f, 0.6f, 1.0f, 1.0f);
        renderer.drawQuad({rect_.x, rect_.y}, {rect_.width, rect_.height}, bg);

        float frac = (value_ - minValue_) / (maxValue_ - minValue_);
        float fillWidth = rect_.width * frac;
        renderer.drawQuad({rect_.x, rect_.y}, {fillWidth, rect_.height}, fill);
    }

    bool Slider::onEvent(const struct UIEvent& event)
    {
        if (!visible_ || !enabled_) return false;

        if (event.type == UIEvent::Type::MouseButton && event.mousePressed)
        {
            if (rect_.contains({event.mouseX, event.mouseY}))
            {
                dragging_ = true;
                float frac = (event.mouseX - rect_.x) / rect_.width;
                value_ = minValue_ + frac * (maxValue_ - minValue_);
                return true;
            }
        }

        if (event.type == UIEvent::Type::MouseMove && dragging_)
        {
            float frac = (event.mouseX - rect_.x) / rect_.width;
            frac = std::clamp(frac, 0.f, 1.f);
            value_ = minValue_ + frac * (maxValue_ - minValue_);
            return true;
        }

        if (event.type == UIEvent::Type::MouseButton && !event.mousePressed)
        {
            dragging_ = false;
        }

        return false;
    }

    void Slider::update(float dt)
    {
        (void)dt;
    }
}
