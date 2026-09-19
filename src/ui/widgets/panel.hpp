#pragma once

#include "widget.hpp"

namespace xel::ui
{
    class Panel : public Widget
    {
    public:
        Panel(const glm::vec2& position, const glm::vec2& size);

        void render(class UIRenderer& renderer) override;
        bool onEvent(const struct UIEvent& event) override;

    private:
        glm::vec4 bgColor_ = {0.15f, 0.15f, 0.18f, 0.95f};
    };
}
