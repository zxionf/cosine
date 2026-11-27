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
    const glm::vec4 color = input.hovered ? glm::vec4{1, 0, 1, 1} : glm::vec4{0, 1, 0, 1};
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
    if (!input.focused) return false;
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
    const std::uint64_t revision = context() != nullptr ? context()->theme_revision() : 0;

    // 主题变化（切换主题/圆角/密度/过渡时长）：所有槽重建，不做过渡。
    // 逐槽重建（UI-38）：旧实现用单槽 + 整对象赋值，一次主题变化只刷新最后一个角色。
    StyleTransition& transition = visual.transition;
    if (transition.revision != revision) {
        for (auto& slot : transition.slots) slot = StyleSlot{};
        transition.revision = revision;
    }

    StyleSlot& slot = transition.slot(role);
    const auto style_of = [&](ControlState s) { return theme_value.style(role, s); };

    // 首次使用：吸附到目标状态（不做淡入）。与旧实现一致。
    if (!slot.started) {
        slot.from = state;
        slot.to = state;
        slot.started = true;
        slot.reversing = false;
        slot.progress = 1.0f;
        return style_of(state);
    }
    if (slot.to != state) {
        const bool back_to_from = slot.from == state && slot.progress < 1.0f;
        if (back_to_from) {
            // 反向：保持 from/to 不变，只把进度往回走 → 中途反向不跳变。
            slot.reversing = true;
        } else {
            // 走向新目标：起点取上一个目标状态（旧实现取“当前插值色”；
            // 见 widget_style.hpp 里关于“第三个状态被打断”的近似说明）。
            slot.from = slot.to;
            slot.to = state;
            slot.reversing = false;
            slot.progress = transition_duration() <= 0.0f ? 1.0f : 0.0f;
        }
    } else if (slot.reversing) {
        // 反向途中目标又变回 to：继续正向走。
        slot.reversing = false;
    }
    return lerp_style(style_of(slot.from), style_of(slot.to), ease_out_quad(slot.progress));
}

bool Widget::transition_active() const
{
    return visual.transition.any_active();
}

float Widget::transition_duration() const
{
    const float ms = ui_theme().transition_ms;
    return ms > 0.0f ? ms / 1000.0f : 0.0f;
}

BoxStyle Widget::current_transition_style(StyleRole role) const
{
    const StyleSlot& slot = visual.transition.slot(role);
    if (!slot.started) return ui_theme().style(role, ControlState::Normal);
    return lerp_style(ui_theme().style(role, slot.from), ui_theme().style(role, slot.to),
                      ease_out_quad(slot.progress));
}

void Widget::advance_transition(float dt)
{
    const float duration = transition_duration();
    for (auto& slot : visual.transition.slots) {
        if (!slot.started || slot.progress >= 1.0f) continue;
        if (duration <= 0.0f) {
            slot.progress = 1.0f;
        } else if (slot.reversing) {
            slot.progress = std::max(0.0f, slot.progress - dt / duration);
            if (slot.progress <= 0.0f) {
                // 回到起点：settle 成 from == to == from，后续按普通状态处理。
                slot.to = slot.from;
                slot.reversing = false;
                slot.progress = 1.0f;
            }
        } else {
            slot.progress = std::min(1.0f, slot.progress + dt / duration);
        }
    }
}
} // namespace xel::ui
