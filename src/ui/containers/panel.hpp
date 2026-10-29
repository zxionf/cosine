#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/containers/vbox.hpp"
#include "graphics/box.hpp"

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

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = t.style(StyleRole::Surface, ControlState::Normal);
        const float radius = corner_radius(style.radius);

        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            // hover 时用强调色提示可交互（与迁移前一致）。
            const glm::vec4 border_color = is_hovered() ? t.accent : style.border;
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, border_color);
        }
        graphics::append_inset_shadow(list, bounds, radius, t.inset_shadow_size, t.inset_shadow);

        list.push_clip(bounds);
        Vbox::append_to(list);
        list.pop_clip();
    }
};
} // namespace xel::ui
