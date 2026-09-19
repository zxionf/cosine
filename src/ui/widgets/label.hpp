#pragma once

#include "widget.hpp"
#include "../font/font_atlas.hpp"

namespace xel::ui
{
    class Label : public Widget
    {
    public:
        Label(const std::string& text, const glm::vec2& position, float fontSize = 16.f);

        void render(class UIRenderer& renderer) override;
        bool onEvent(const struct UIEvent& event) override;

        void setText(const std::string& text);
        const std::string& getText() const { return text_; }

    private:
        std::string text_;
        float fontSize_;
        FontAtlas* fontAtlas_ = nullptr;
    };
}
