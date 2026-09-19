#pragma once

#include <functional>
#include "widget.hpp"
#include "font/font_atlas.hpp"

namespace xel::ui
{
    class Button : public Widget
    {
    public:
        using Callback = std::function<void()>;

        Button(const std::string& label, const Rect& rect, Callback callback = nullptr);

        void render(class UIRenderer& renderer) override;
        bool onEvent(const struct UIEvent& event) override;
        void update(float dt) override;

        void setCallback(Callback cb) { callback_ = cb; }

    private:
        std::string label_;
        Callback callback_;
        float hoverTime_ = 0.f;
        FontAtlas* fontAtlas_ = nullptr;
    };
}
