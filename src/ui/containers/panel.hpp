#pragma once

#include "ui/style/theme.hpp"
#include "ui/style/style.hpp"
#include "ui/system/uicontext.hpp"
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
        apply_theme_padding(ui_theme().padding);
        apply_theme_spacing(ui_theme().spacing);
    }

    explicit Panel(glm::vec2 size) : Panel(0.0f, 0.0f, size.x, size.y) {}

    void set_context(UIContext* context) override
    {
        Widget::set_context(context);
        if (context) {
            // 只对没显式设过 padding 的面板套主题值：贴满父容器的页面（例如窗口里的
            // ScrollPanel 页）用 set_padding(0) 锁住，主题切换后不会再长回 8px。
            apply_theme_padding(context->theme().padding);
            apply_theme_spacing(context->theme().spacing);
        }
    }

    bool hit_testable() const override { return true; }
    // append_to 已把子项裁剪到自身 bounds；命中必须同步裁剪，否则不可见的子项仍可点击。
    Overflow overflow() const override { return Overflow::Clip; }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        // UI-52：hover 不再手写三元式，统一走 Interaction + Surface:Hovered。
        const BoxStyle style = visual_style(StyleRole::Surface,
                                            Interaction{true, is_hovered(), false}.state());
        const float radius = corner_radius(style.radius);

        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, style.border);
        }
        graphics::append_inset_shadow(list, bounds, radius, t.inset_shadow_size, t.inset_shadow);

        list.push_clip(bounds);
        Vbox::append_to(list);
        list.pop_clip();
    }
};
} // namespace xel::ui
