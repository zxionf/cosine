#pragma once

#include "theme.hpp"
#include "vbox.hpp"
#include "../graphics/rectangle.hpp"

#include <string>

namespace xel::ui
{
class Panel : public Vbox
{
public:
    Panel(float x, float y, float width, float height)
        : Vbox(x, y, width, height)
    {
        set_padding(ui_theme().padding);
        set_spacing(ui_theme().spacing);
    }

    explicit Panel(glm::vec2 size) : Panel(0.0f, 0.0f, size.x, size.y) {}

    void set_context(UIContext* context) override
    {
        Widget::set_context(context);
        if (context) {
            set_padding(context->theme.padding);
            set_spacing(context->theme.spacing);
        }
    }

    bool hit_testable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();

        graphics::Rectangle background{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y, t.panel_bg};
        background.append_to(list);

        if (t.border > 0.0f) {
            const glm::vec4 border_color = hovered_ ? t.focus_ring : t.panel_border;
            const float b = t.border;
            graphics::Rectangle top   {global_.pos.x, global_.pos.y, global_.size.x, b, border_color};
            graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b, global_.size.x, b, border_color};
            graphics::Rectangle left  {global_.pos.x, global_.pos.y, b, global_.size.y, border_color};
            graphics::Rectangle right {global_.pos.x + global_.size.x - b, global_.pos.y, b, global_.size.y, border_color};
            top.append_to(list);
            bottom.append_to(list);
            left.append_to(list);
            right.append_to(list);
        }

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        Vbox::append_to(list);
        list.pop_clip();
    }

private:
    bool hovered_ = false;
};
} // namespace xel::ui
