#include "ui/widget.hpp"

#include "graphics/drawable.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/uicontext.hpp"
#include "ui/widget_style.hpp"

#include <algorithm>

namespace xel::ui
{
Widget::Widget() = default;

Widget::~Widget() = default;

void Widget::append_debug_tree(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    for (const auto& child : children()) child->append_debug_tree(list);
    for (const auto& overlay : overlays()) overlay->append_debug_tree(list);
}

void Widget::debug_rect(graphics::DrawList& list) const
{
    auto [x, y, w, h] = global_;
    const glm::vec4 color = hovered_ ? glm::vec4{1, 0, 1, 1} : glm::vec4{0, 1, 0, 1};
    graphics::Line top_l    {x,     y,      x + w,  y,      1.0f, color};
    graphics::Line right_l  {x + w, y,      x + w,  y + h,  1.0f, color};
    graphics::Line bottom_l {x,     y + h,  x + w,  y + h,  1.0f, color};
    graphics::Line left_l   {x,     y,      x,      y + h,  1.0f, color};
    top_l.append_to(list);
    right_l.append_to(list);
    bottom_l.append_to(list);
    left_l.append_to(list);
}

const Theme& Widget::ui_theme() const
{
    if (auto* ctx = context()) return ctx->theme;
    return theme();
}

graphics::FontAtlas* Widget::font_atlas() const
{
    if (auto* ctx = context()) return ctx->font_atlas;
    return graphics::active_font_atlas();
}

bool Widget::focus_visible() const
{
    if (!focused_) return false;
    const UIContext* ctx = context();
    if (ctx == nullptr) return true;
    switch (ctx->theme.focus_policy) {
        case FocusPolicy::Always: return true;
        case FocusPolicy::Never: return false;
        case FocusPolicy::Keyboard: break;
    }
    return ctx->input_mode == InputMode::Keyboard;
}

void Widget::append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const
{
    if (!focus_visible()) return;
    const auto& t = ui_theme();
    const float thickness = t.focus_thickness;
    if (t.focus_style == FocusStyle::Border) {
        graphics::append_rounded_rect(list, {rect.x, rect.y, rect.z, thickness}, 0.0f, t.accent);
        graphics::append_rounded_rect(list, {rect.x, rect.y + rect.w - thickness, rect.z, thickness}, 0.0f, t.accent);
        graphics::append_rounded_rect(list, {rect.x, rect.y, thickness, rect.w}, 0.0f, t.accent);
        graphics::append_rounded_rect(list, {rect.x + rect.z - thickness, rect.y, thickness, rect.w}, 0.0f, t.accent);
        return;
    }
    const float offset = t.focus_offset;
    graphics::append_rounded_ring(
        list,
        {rect.x - offset, rect.y - offset, rect.z + offset * 2.0f, rect.w + offset * 2.0f},
        radius + offset,
        thickness,
        t.accent);
}

BoxStyle Widget::visual_style(StyleRole role, ControlState state) const
{
    const auto& theme_value = ui_theme();
    const BoxStyle target = theme_value.style(role, state);
    const std::uint64_t revision = theme_revision();
    if (transition_ == nullptr) transition_ = std::make_unique<StyleTransition>();
    // 主题变化（切换主题/圆角/密度/过渡时长）：旧缓存的 from/to 已过时，
    // 直接重建成目标样式，不做过渡——否则会等到下一次 hover 才刷新。
    if (!transition_->started || transition_->revision != revision) {
        *transition_ = StyleTransition{role, state, target, target, 1.0f, true, revision};
        return target;
    }
    if (transition_->role != role || transition_->state != state) {
        transition_->from = current_transition_style();
        transition_->to = target;
        transition_->role = role;
        transition_->state = state;
        transition_->progress = transition_duration() <= 0.0f ? 1.0f : 0.0f;
    }
    return current_transition_style();
}

bool Widget::transition_active() const
{
    return transition_ != nullptr && transition_->progress < 1.0f;
}

float Widget::transition_duration() const
{
    const float ms = ui_theme().transition_ms;
    return ms > 0.0f ? ms / 1000.0f : 0.0f;
}

BoxStyle Widget::current_transition_style() const
{
    if (transition_ == nullptr) {
        return ui_theme().style(StyleRole::Control, ControlState::Normal);
    }
    return lerp_style(transition_->from, transition_->to, ease_out_quad(transition_->progress));
}

void Widget::advance_transition(float dt)
{
    if (transition_ == nullptr || transition_->progress >= 1.0f) return;
    const float duration = transition_duration();
    if (duration <= 0.0f) {
        transition_->progress = 1.0f;
        return;
    }
    transition_->progress = std::min(1.0f, transition_->progress + dt / duration);
}
} // namespace xel::ui
