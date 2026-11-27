#include "ui/node/node.hpp"

#include "graphics/box.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/line.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/uicontext.hpp"

#include <algorithm>

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
