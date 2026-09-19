#pragma once

#include "widget.hpp"
#include "ui_window.hpp"

namespace xel::ui
{
    class TextInput : public Widget
    {
    public:
        TextInput(const glm::vec2& position, const std::string& placeholder = "");

        void render(class UIRenderer& renderer) override;
        bool onEvent(const struct UIEvent& event) override;
        void update(float dt) override;

        const std::string& getText() const { return text_; }
        void setText(const std::string& t) { text_ = t; }

    private:
        std::string text_;
        std::string placeholder_;
        bool focused_ = false;
        float cursorBlink_ = 0.f;
    };
}
