#include "ui/node/node.hpp"

#include "graphics/box.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/line.hpp"
#include "ui/style/theme.hpp"
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

// ── 绘制/命中顺序 ──────────────────────────────────────────────────
std::vector<Node*> Node::sorted_paint_order(const std::vector<std::unique_ptr<Node>>& list)
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

std::vector<Node*> Node::paint_order() const
{
    return sorted_paint_order(children());
}

std::vector<Node*> Node::overlay_paint_order() const
{
    return sorted_paint_order(overlays());
}

// ── 递归失效 ────────────────────────────────────────────────────────
void Node::invalidate_measure_recursive()
{
    invalidate_measure();
    for_each_child([](Node& child) { child.invalidate_measure_recursive(); });
}

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

std::vector<Node*> Node::layer_paint_order() const
{
    std::vector<Node*> order;
    order.reserve(children().size() + overlays().size());
    for_each_child([&order](Node& child) { order.push_back(&child); });
    std::stable_sort(order.begin(), order.end(), [](Node* a, Node* b) {
        if (a->layer() != b->layer()) return a->layer() < b->layer();
        return a->z_index() < b->z_index();
    });
    return order;
}

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
    auto [x, y, w, h] = global_;
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
} // namespace xel::ui
