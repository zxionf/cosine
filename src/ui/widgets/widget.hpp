#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include "../layout/layout.hpp"
#include "../theme/theme.hpp"

namespace xel::ui
{
    struct UIEvent;
    enum class WidgetState
    {
        Normal,
        Hovered,
        Active,
        Disabled
    };

    class Widget
    {
    public:
        using Ptr = std::shared_ptr<Widget>;
        using Ref = std::weak_ptr<Widget>;

        Widget(const std::string& name = "");
        virtual ~Widget() = default;

        virtual void render(class UIRenderer& renderer) = 0;
        virtual bool onEvent(const struct UIEvent& event) = 0;
        virtual void update(float dt) {}

        const Rect& getRect() const { return rect_; }
        void setRect(const Rect& r) { rect_ = r; }
        const std::string& getName() const { return name_; }
        bool isVisible() const { return visible_; }
        void setVisible(bool v) { visible_ = v; }
        bool isEnabled() const { return enabled_; }
        void setEnabled(bool e) { enabled_ = e; }

        void setParent(Widget* parent) { parent_ = parent; }
        Widget* getParent() { return parent_; }

        virtual WidgetState getState() const { return state_; }

    protected:
        std::string name_;
        Rect rect_;
        WidgetState state_ = WidgetState::Normal;
        bool visible_ = true;
        bool enabled_ = true;
        Widget* parent_ = nullptr;
        std::vector<Ptr> children_;
    };
}
