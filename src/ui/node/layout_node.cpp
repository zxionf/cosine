#include "ui/node/layout_node.hpp"

#include "graphics/box.hpp"
#include "graphics/font_atlas.hpp"
#include "ui/geom/layout_metrics.hpp"
#include "ui/style/style.hpp"
#include "ui/style/theme.hpp"
#include "ui/style/widget_style.hpp"
#include "ui/system/uicontext.hpp"

#include <algorithm>

namespace xel::ui
{
LayoutNode::LayoutNode() = default;

LayoutNode::~LayoutNode() = default;

void LayoutNode::set_absolute(bool value)
{
    // 从当前位置开始（兼容旧行为）：仅当从“非 Positioned”切到 Positioned 时同步坐标。
    const bool take_current = value && display() != Display::Positioned;
    if (take_current) layout_.absolute_pos = local_.pos;
    set_display(value ? Display::Positioned : Display::Flow);
}

bool LayoutNode::focus_visible() const
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

void LayoutNode::append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const
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

// ── 布局契约实现 ────────────────────────────────────────────────────
void LayoutNode::mark_layout_dirty()
{
    if (!in_layout_pass()) layout_dirty_ = true;
    invalidate_measure();
    if (notified_) return;
    notified_ = true;
    if (parent_ != nullptr) parent_->mark_layout_dirty();
}

void LayoutNode::clear_layout_dirty_recursive()
{
    layout_dirty_ = false;
    notified_ = false;
    for_each_child([](Node& child) { child.clear_layout_dirty_recursive(); });
}

bool LayoutNode::measure_cache_hit(glm::vec2 constraints) const
{
    MeasureKey probe;
    probe.constraints = constraints;
    probe.epoch = layout_epoch();
    return measure_cache_.hit(probe);
}

void LayoutNode::store_measure(glm::vec2 constraints, glm::vec2 result)
{
    MeasureKey probe;
    probe.constraints = constraints;
    probe.epoch = layout_epoch();
    measure_cache_.store(probe, result);
}

glm::vec2 LayoutNode::measure_child(Node& child, glm::vec2 constraints)
{
    return measure_child(static_cast<LayoutNode&>(child), constraints);
}

glm::vec2 LayoutNode::measure_child(LayoutNode& child, glm::vec2 constraints)
{
    if (child.measure_cache_hit(constraints)) return child.measure_cached();

    const auto available_axis = [](float extent, float before, float after) {
        if (!is_bounded(extent)) return extent; // 无界轴保持无界，否则减完 margin 会变成“有界”
        return std::max(0.0f, extent - before - after);
    };
    const glm::vec2 available{
        available_axis(constraints.x, child.margin_left(), child.margin_right()),
        available_axis(constraints.y, child.margin_top(), child.margin_bottom())};

    const glm::vec2 pct = child.size_percent();
    const bool fill_x = child.fill_x() && is_bounded(available.x);
    const bool fill_y = child.fill_y() && is_bounded(available.y);
    const bool pct_x = !fill_x && pct.x > 0.0f && is_bounded(available.x);
    const bool pct_y = !fill_y && pct.y > 0.0f && is_bounded(available.y);

    glm::vec2 spec = available;
    if (pct_x) spec.x = available.x * pct.x;
    if (pct_y) spec.y = available.y * pct.y;

    glm::vec2 size = child.measure(spec);
    if (fill_x || pct_x) size.x = spec.x;
    if (fill_y || pct_y) size.y = spec.y;

    glm::vec2 result = child.clamp_size(size);
    if (const float ratio = child.aspect_ratio(); ratio > 0.0f) {
        if (pct_y && !pct_x) size.x = size.y * ratio;
        else size.y = size.x / ratio;
        // aspect 与 clamp 一起求解（UI-32）：clamp 截断任一轴后回算另一轴保持比例。
        result = apply_aspect_clamp(size, ratio, child.min_size(), child.max_size());
    }
    child.mark_measured(); // 已测量：失效传播早退的不变量
    child.store_measure(constraints, result);
    return result;
}

// ── 样式过渡 ────────────────────────────────────────────────────────
BoxStyle LayoutNode::visual_style(StyleRole role, ControlState state) const
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
            // 走向新目标：起点取上一个目标状态（见 widget_style.hpp 的近似说明）。
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

bool LayoutNode::transition_active() const
{
    return visual.transition.any_active();
}

float LayoutNode::transition_duration() const
{
    const float ms = ui_theme().transition_ms;
    return ms > 0.0f ? ms / 1000.0f : 0.0f;
}

BoxStyle LayoutNode::current_transition_style(StyleRole role) const
{
    const StyleSlot& slot = visual.transition.slot(role);
    if (!slot.started) return ui_theme().style(role, ControlState::Normal);
    return lerp_style(ui_theme().style(role, slot.from), ui_theme().style(role, slot.to),
                      ease_out_quad(slot.progress));
}

void LayoutNode::advance_transition(float dt)
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
