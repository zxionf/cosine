#include "ui/widget.hpp"

#include "graphics/drawable.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/uicontext.hpp"
#include "ui/widget_style.hpp"

namespace xel::ui
{
namespace
{
// 布局阶段深度：>0 时 mark_layout_dirty 只失效缓存、不重新标记脏。
int g_layout_pass_depth = 0;
// 布局轮次：每轮 UIRoot::layout() 推进一次（UI-34）。
std::uint64_t g_layout_epoch = 1;
} // namespace

std::uint64_t layout_epoch()
{
    return g_layout_epoch;
}

void advance_layout_epoch()
{
    ++g_layout_epoch;
}

bool in_layout_pass()
{
    return g_layout_pass_depth > 0;
}

LayoutPassScope::LayoutPassScope()
{
    ++g_layout_pass_depth;
}

LayoutPassScope::~LayoutPassScope()
{
    --g_layout_pass_depth;
}

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
    if (auto* ctx = context()) return ctx->theme();
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
    switch (ctx->theme().focus_policy) {
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
    // revision 取本 context 的（UI-42）：多 root / lab / 测试各自一轮，不互相打断缓存。
    const std::uint64_t revision = context() != nullptr ? context()->theme_revision() : 0;
    if (transition_ == nullptr) transition_ = std::make_unique<StyleTransition>();

    // 主题变化（切换主题/圆角/密度/过渡时长）：所有槽重建为目标样式，不做过渡。
    // 逐槽重建（关键）：旧实现用单槽 + 整对象赋值，一次主题变化只会刷新最后一个
    // 被请求的角色，其余角色留着旧主题的颜色。
    if (transition_->revision != revision) {
        for (auto& slot : transition_->slots) slot = StyleSlot{};
        transition_->revision = revision;
    }

    StyleSlot& slot = transition_->slot(role);
    if (!slot.started) {
        slot = StyleSlot{state, target, target, 1.0f, true};
        return target;
    }
    if (slot.state != state) {
        // 从本槽自己的当前插值结果出发，不会跨角色取色。
        slot.from = lerp_style(slot.from, slot.to, ease_out_quad(slot.progress));
        slot.to = target;
        slot.state = state;
        slot.progress = transition_duration() <= 0.0f ? 1.0f : 0.0f;
    }
    return lerp_style(slot.from, slot.to, ease_out_quad(slot.progress));
}

bool Widget::transition_active() const
{
    return transition_ != nullptr && transition_->any_active();
}

float Widget::transition_duration() const
{
    const float ms = ui_theme().transition_ms;
    return ms > 0.0f ? ms / 1000.0f : 0.0f;
}

BoxStyle Widget::current_transition_style(StyleRole role) const
{
    if (transition_ == nullptr) {
        return ui_theme().style(role, ControlState::Normal);
    }
    const StyleSlot& slot = transition_->slot(role);
    if (!slot.started) return ui_theme().style(role, ControlState::Normal);
    return lerp_style(slot.from, slot.to, ease_out_quad(slot.progress));
}

void Widget::advance_transition(float dt)
{
    if (transition_ == nullptr) return;
    const float duration = transition_duration();
    for (auto& slot : transition_->slots) {
        if (!slot.started || slot.progress >= 1.0f) continue;
        if (duration <= 0.0f) {
            slot.progress = 1.0f;
            continue;
        }
        slot.progress = std::min(1.0f, slot.progress + dt / duration);
    }
}
} // namespace xel::ui
