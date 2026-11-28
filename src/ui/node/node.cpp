#include "ui/node/node.hpp"

#include "graphics/box.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/line.hpp"
#include "ui/style/painters.hpp"
#include "ui/style/style.hpp"
#include "ui/style/theme.hpp"
#include "ui/style/widget_style.hpp"
#include "ui/system/uicontext.hpp"

#include <algorithm>
#include <iterator>

namespace xel::ui
{
namespace
{
// 布局阶段深度：>0 时 mark_layout_dirty 只失效缓存、不重新标记脏。
int g_layout_pass_depth = 0;
// 布局轮次：每轮 UIRoot::layout() 推进一次。
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

Node::Node() = default;

Node::~Node() = default;

const Theme& Node::ui_theme() const
{
    if (auto* ctx = context()) return ctx->theme();
    return theme();
}

graphics::FontAtlas* Node::font_atlas() const
{
    if (auto* ctx = context()) return ctx->font_atlas;
    return graphics::active_font_atlas();
}

// ── 树：容纳子节点是 Node 的能力 ────────────────────────────────────
void Node::adopt(std::unique_ptr<Node> child)
{
    child->set_parent(this);
    child->set_context(context());
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void Node::clear_children()
{
    children_.clear();
    mark_layout_dirty();
}

std::unique_ptr<Node> Node::detach_child(std::size_t index)
{
    if (index >= children_.size()) return nullptr;
    auto owned = std::move(children_[index]);
    children_.erase(children_.begin() + static_cast<std::ptrdiff_t>(index));
    mark_layout_dirty();
    return owned;
}

void Node::replace_child(std::size_t index, std::unique_ptr<Node> child)
{
    if (index >= children_.size() || !child) return;
    child->set_parent(this);
    child->set_context(context());
    children_[index] = std::move(child);
    mark_layout_dirty();
}

void Node::raise_child(Node* child)
{
    if (!child) return;
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() != child) continue;
        if (std::next(it) == children_.end()) return;
        auto owned = std::move(*it);
        children_.erase(it);
        children_.push_back(std::move(owned));
        mark_layout_dirty();
        return;
    }
}

// 排序与命中（layer 升序 → z 升序；稳定）—— 实现细节，不进头文件。
namespace
{
std::vector<Node*> sorted_paint_order(const std::vector<std::unique_ptr<Node>>& list)
{
    std::vector<Node*> order;
    order.reserve(list.size());
    for (const auto& item : list) order.push_back(item.get());
    if (std::any_of(order.begin(), order.end(), [](Node* n) { return n->z_index() != 0; })) {
        std::stable_sort(order.begin(), order.end(),
                         [](Node* a, Node* b) { return a->z_index() < b->z_index(); });
    }
    return order;
}
} // namespace

void Node::set_context(UIContext* context)
{
    context_ = context;
    for_each_child([context](Node& child) { child.set_context(context); });
}

// ── 可见性 ─────────────────────────────────────────────────────────
void Node::set_display(Display value)
{
    display_ = value == Display::Hidden ? Display::Flow : value;
    visible_ = value != Display::Hidden;
    mark_layout_dirty();
}

// 绘制/命中顺序：paint_order 只按 z（同层子项用），layer_paint_order 再叠上 layer 优先级。
std::vector<Node*> Node::paint_order() const
{
    return sorted_paint_order(children());
}

std::vector<Node*> Node::layer_paint_order() const
{
    std::vector<Node*> order;
    order.reserve(children_.size());
    for (const auto& child : children_) order.push_back(child.get());
    std::stable_sort(order.begin(), order.end(), [](Node* a, Node* b) {
        if (a->layer() != b->layer()) return a->layer() < b->layer();
        return a->z_index() < b->z_index();
    });
    return order;
}

// ── 语义投影：状态从 Interaction 现读 ────────────────────────────────
Semantics Node::semantics() const
{
    Semantics s;
    s.role = semantics_role();
    s.label = semantics_label();
    s.enabled = input.enabled;
    s.focused = input.focused;
    s.selected = input.selected;
    s.checkable = role_is_checkable(s.role);
    s.editable = role_is_editable(s.role);
    s.expanded = semantics_expanded();
    return s;
}

void Node::set_enabled(bool enabled)
{
    input.enabled = enabled;
    mark_layout_dirty();
}

// ── 布局意图：LayoutSpec 是唯一的快照形态，写入只经 set_layout_spec ─
void Node::set_align(Align a)
{
    LayoutSpec spec = layout_spec();
    spec.align = a;
    set_layout_spec(spec);
}

Node::LayoutSpec Node::layout_spec() const
{
    return {align_self(), layout_.flex_grow, layout_.size_percent, layout_.aspect_ratio,
            layout_.fill_x, layout_.fill_y, layout_.min_size, layout_.max_size,
            layout_.preferred};
}

void Node::set_layout_spec(const LayoutSpec& spec)
{
    layout_.align_self = spec.align.value_or(Align::top_left());
    layout_.has_align = spec.align.has_value();
    layout_.flex_grow = spec.flex_grow < 0.0f ? 0.0f : spec.flex_grow;
    layout_.size_percent = spec.size_percent;
    layout_.aspect_ratio = spec.aspect_ratio > 0.0f ? spec.aspect_ratio : 0.0f;
    layout_.fill_x = spec.fill_x;
    layout_.fill_y = spec.fill_y;
    layout_.min_size = spec.min_size;
    layout_.max_size = spec.max_size;
    layout_.preferred = spec.preferred;
    mark_layout_dirty();
}

// ── 盒模型 ──────────────────────────────────────────────────────────
void Node::set_padding(Edges edges)
{
    box.padding = edges;
    box.padding_override = true;
    mark_layout_dirty();
}

void Node::apply_theme_padding(float all)
{
    if (box.padding_override) return;
    box.padding = {all, all, all, all};
    mark_layout_dirty();
}

void Node::set_margin(Edges edges)
{
    box.margin = edges;
    mark_layout_dirty();
}

// ── 尺寸意图 ────────────────────────────────────────────────────────
void Node::set_flex_grow(float grow)
{
    LayoutSpec spec = layout_spec();
    spec.flex_grow = grow;
    set_layout_spec(spec);
}

void Node::set_absolute_pos(glm::vec2 pos)
{
    layout_.absolute_pos = pos;
    set_display(Display::Positioned);
}

void Node::set_min_size(glm::vec2 size)
{
    LayoutSpec spec = layout_spec();
    spec.min_size = size;
    set_layout_spec(spec);
}

void Node::set_max_size(glm::vec2 size)
{
    LayoutSpec spec = layout_spec();
    spec.max_size = size;
    set_layout_spec(spec);
}

void Node::set_preferred_size(glm::vec2 size)
{
    LayoutSpec spec = layout_spec();
    spec.preferred = size;
    set_layout_spec(spec);
}

void Node::request_size(glm::vec2 size)
{
    set_preferred_size(size);
    set_size(size);
}

void Node::set_size_percent(glm::vec2 pct)
{
    LayoutSpec spec = layout_spec();
    spec.size_percent = pct;
    set_layout_spec(spec);
}

void Node::set_fill(bool x, bool y)
{
    LayoutSpec spec = layout_spec();
    spec.fill_x = x;
    spec.fill_y = y;
    set_layout_spec(spec);
}

void Node::set_aspect_ratio(float ratio)
{
    LayoutSpec spec = layout_spec();
    spec.aspect_ratio = ratio;
    set_layout_spec(spec);
}

// ── 内容溢出 ────────────────────────────────────────────────────────
bool Node::content_overflows() const
{
    constexpr float kEps = 0.5f;
    return content_extent_.x > bounds_.size.x + kEps
        || content_extent_.y > bounds_.size.y + kEps;
}

void Node::set_absolute(bool value)
{
    // 从当前位置开始（兼容旧行为）：仅当从“非 Positioned”切到 Positioned 时同步坐标。
    const bool take_current = value && display() != Display::Positioned;
    if (take_current) layout_.absolute_pos = bounds_.pos;
    set_display(value ? Display::Positioned : Display::Flow);
}

bool Node::focus_visible() const
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

// 实现只有一份（style/painters.hpp），这里是给「环心不是控件 bounds」的控件
// （Slider 手柄、表头、菜单项）保留的便捷入口。
void Node::append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const
{
    paint::focus_ring(list, Rect{{rect.x, rect.y}, {rect.z, rect.w}}, radius, ui_theme(),
                      focus_visible());
}

// ── 布局契约实现 ────────────────────────────────────────────────────
void Node::mark_layout_dirty()
{
    if (!in_layout_pass()) layout_dirty_ = true;
    invalidate_measure();
    if (notified_) return;
    notified_ = true;
    if (parent_ != nullptr) parent_->mark_layout_dirty();
}

void Node::invalidate_measure_recursive()
{
    invalidate_measure();
    for_each_child([](Node& child) { child.invalidate_measure_recursive(); });
}

void Node::clear_layout_dirty_recursive()
{
    layout_dirty_ = false;
    notified_ = false;
    for_each_child([](Node& child) { child.clear_layout_dirty_recursive(); });
}

bool Node::measure_cache_hit(glm::vec2 constraints) const
{
    MeasureKey probe;
    probe.constraints = constraints;
    probe.epoch = layout_epoch();
    return measure_cache_.hit(probe);
}

void Node::store_measure(glm::vec2 constraints, glm::vec2 result)
{
    MeasureKey probe;
    probe.constraints = constraints;
    probe.epoch = layout_epoch();
    measure_cache_.store(probe, result);
}

glm::vec2 Node::measure_child(Node& child, glm::vec2 constraints)
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
        // aspect 与 clamp 一起求解：clamp 截断任一轴后回算另一轴保持比例。
        result = apply_aspect_clamp(size, ratio, child.min_size(), child.max_size());
    }
    child.mark_measured(); // 已测量：失效传播早退的不变量
    child.store_measure(constraints, result);
    return result;
}

// ── 调试 ────────────────────────────────────────────────────────────
void Node::debug_recursive()
{
    debug();
    for_each_child([](Node& child) { child.debug_recursive(); });
}

void Node::append_debug_tree(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    for_each_child([&list](const Node& child) { child.append_debug_tree(list); });
}

void Node::debug_rect(graphics::DrawList& list) const
{
    auto [x, y, w, h] = bounds_;
    const glm::vec4 color = debug_ ? glm::vec4{1, 0, 1, 1} : glm::vec4{0, 1, 0, 1};
    graphics::Line top_l    {x,     y,      x + w,  y,      1.0f, color};
    graphics::Line right_l  {x + w, y,      x + w,  y + h,  1.0f, color};
    graphics::Line bottom_l {x,     y + h,  x + w,  y + h,  1.0f, color};
    graphics::Line left_l   {x,     y,      x,      y + h,  1.0f, color};
    top_l.append_to(list);
    right_l.append_to(list);
    bottom_l.append_to(list);
    left_l.append_to(list);
}

// ── 样式过渡 ────────────────────────────────────────────────────────
BoxStyle Node::visual_style(StyleRole role, ControlState state) const
{
    const auto& theme_value = ui_theme();
    const std::uint64_t revision = context() != nullptr ? context()->theme_revision() : 0;

    // 主题变化（切换主题/圆角/密度/过渡时长）：所有槽重建，不做过渡。
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

bool Node::transition_active() const
{
    return visual.transition.any_active();
}

float Node::transition_duration() const
{
    const float ms = ui_theme().transition_ms;
    return ms > 0.0f ? ms / 1000.0f : 0.0f;
}

BoxStyle Node::current_transition_style(StyleRole role) const
{
    const StyleSlot& slot = visual.transition.slot(role);
    if (!slot.started) return ui_theme().style(role, ControlState::Normal);
    return lerp_style(ui_theme().style(role, slot.from), ui_theme().style(role, slot.to),
                      ease_out_quad(slot.progress));
}

void Node::advance_transition(float dt)
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
