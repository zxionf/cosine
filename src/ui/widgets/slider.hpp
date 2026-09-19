#pragma once

#include "widget.hpp"
#include "ui_window.hpp"

namespace xel::ui
{
    class Slider : public Widget
    {
    public:
        Slider(const glm::vec2& position, float minVal, float maxVal, float defaultValue = 0.5f);

        void render(class UIRenderer& renderer) override;
        bool onEvent(const struct UIEvent& event) override;
        void update(float dt) override;

        float getValue() const { return value_; }
        void setValue(float v) { value_ = v; }

    private:
        float minValue_;
        float maxValue_;
        float value_;
        bool dragging_ = false;
    };
}
