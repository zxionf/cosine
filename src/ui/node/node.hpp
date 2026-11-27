#pragma once

#include "events/key_code.hpp"
#include "graphics/drawable.hpp"
#include "ui/geom/rect.hpp"
#include "ui/layout/layout_params.hpp" // Display
#include "ui/semantics/semantics.hpp"
#include "ui/style/style_types.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <algorithm>

namespace xel::graphics
{
class FontAtlas;
class DrawList;
} // namespace xel::graphics

namespace xel::ui
{
struct UIContext;
struct Theme;
struct BoxStyle;
class LayoutNode;

// ── 布局阶段与轮次 ────────────────────────────────
// 布局阶段守卫：UIRoot::layout() 期间生效。
// 目的：把“布局写入”（arrange 里 set_geometry）与“失效请求”（外部改尺寸）区分开。
bool in_layout_pass();

class LayoutPassScope
{
public:
    LayoutPassScope();
    ~LayoutPassScope();
    LayoutPassScope(const LayoutPassScope&) = delete;
    LayoutPassScope& operator=(const LayoutPassScope&) = delete;
};

// 布局轮次：UIRoot::layout() 每轮推进一次；测量缓存以它为跨轮失效键。
std::uint64_t layout_epoch();
void advance_layout_epoch();

// ── Node（设计 §4.1）：身份 / 几何 / 树 / 可见性 / 层叠 / 语义 ────────
//
// 这是整棵 UI 树**唯一的元素类型**（`children_` 里装的是 Node）。
// 布局机制（measure/arrange、布局意图、测量缓存、脏标记）在 LayoutNode 里，
// 但它们的**契约**在这里以纯虚形式声明 —— 这样任何遍历代码都能直接
// `child->measure()` / `child->mark_layout_dirty()`，不需要向下转型。
class Node : public graphics::Drawable
{
public:
    Node();
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    ~Node() override;

    graphics::TextureSlot texture_slot() const override { return graphics::TextureSlot::Texture; }

    // ── 几何：读随处可读，写只经 set_geometry（K3）──────────────────
    float x()       const { return local_.pos.x; }
    float y()       const { return local_.pos.y; }
    float width()   const { return local_.size.x; }
    float height()  const { return local_.size.y; }
    Rect local_bounds()  const { return local_; }
    Rect global_bounds() const { return global_; }

    void set_pos(float x, float y) { local_.pos = {x, y}; mark_layout_dirty(); }
    void set_size(float width, float height) { local_.size = {width, height}; mark_layout_dirty(); }
    void set_pos(glm::vec2 pos) { local_.pos = pos; mark_layout_dirty(); }
    void set_size(glm::vec2 size) { local_.size = size; mark_layout_dirty(); }
    void set_global_bounds(Rect bounds) { global_ = bounds; }

    // 布局写入入口（只能在 arrange 内用）：只改几何，不触发脏标记。
    void set_geometry(Rect slot)
    {
        local_ = slot;
        global_ = slot;
    }
    // 固定尺寸控件的变体：只跟随位置，尺寸由自己维护（Window / Splitter / Table）。
    void set_arranged_pos(glm::vec2 pos)
    {
        local_.pos = pos;
        global_.pos = pos;
    }

    // ── 树：存储私有，遍历只有 for_each_child 一条路（K2）─────────────
    // 非容器控件调用 add() 会抛异常（“是不是容器”无处藏身）。
    virtual void add(std::unique_ptr<Node> child)
    {
        (void)child;
        throw std::logic_error("Node::add is not supported by this node");
    }
    const std::vector<std::unique_ptr<Node>>& children() const { return children_; }
    // overlay 是顶层绘制/命中层：先画普通子项，再画 overlay；命中反序。
    virtual const std::vector<std::unique_ptr<Node>>& overlays() const
    {
        static const std::vector<std::unique_ptr<Node>> empty;
        return empty;
    }
    Node* parent() const { return parent_; }
    void set_parent(Node* parent) { parent_ = parent; }

    std::size_t child_count() const { return children_.size(); }
    Node* child_at(std::size_t index) const
    {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

    // 唯一合法的子项挂载入口：设 parent / context 并加入 children_。
    void adopt(std::unique_ptr<Node> child)
    {
        child->set_parent(this);
        child->set_context(context());
        children_.push_back(std::move(child));
        mark_layout_dirty();
    }
    // 清空子项（销毁）。公开：与 add()/adopt() 同级（容器内容整体重建）。
    void clear_children()
    {
        children_.clear();
        mark_layout_dirty();
    }
    // 替换第 index 个子项（TabView 换页用）。
    void replace_child(std::size_t index, std::unique_ptr<Node> child)
    {
        if (index >= children_.size() || !child) return;
        child->set_parent(this);
        child->set_context(context());
        children_[index] = std::move(child);
        mark_layout_dirty();
    }
    // 按谓词移除子项（模态关闭清 scrim 用）。
    template <class Pred>
    void remove_children_if(Pred pred)
    {
        const std::size_t before = children_.size();
        std::erase_if(children_, pred);
        if (children_.size() != before) mark_layout_dirty();
    }
    // 把已有子项移到末尾（插入序置顶，不改 z_index）。
    void raise_child(Node* child)
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
    // 树的唯一遍历入口：children_ + overlays()。
    template <class F>
    void for_each_child(F&& f) const
    {
        for (const auto& child : children_) f(*child);
        for (const auto& overlay : overlays()) f(*overlay);
    }

    // 生命周期令牌：容器可能直接销毁子控件，UISystem 用 weak_ptr 判断悬垂。
    const std::shared_ptr<const bool>& alive_token() const { return alive_token_; }

    // ── 上下文 / 主题 ────────────────────────────────────────────────
    virtual void set_context(UIContext* context)
    {
        context_ = context;
        for_each_child([context](Node& child) { child.set_context(context); });
    }
    UIContext* context() const
    {
        if (context_) return context_;
        return parent_ ? parent_->context() : nullptr;
    }
    const Theme& ui_theme() const;
    graphics::FontAtlas* font_atlas() const;

    // ── 可见性 / 溢出 / 层叠 ─────────────────────────────────────────
    // 定义在 layout_params.hpp；别名保持 `Widget::Display::Flow` 写法不变。
    using Display = ::xel::ui::Display;

    Display display() const { return !visible_ ? Display::Hidden : display_; }
    void set_display(Display value)
    {
        display_ = value == Display::Hidden ? Display::Flow : value;
        visible_ = value != Display::Hidden;
        mark_layout_dirty();
    }
    bool visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; mark_layout_dirty(); }

    // 溢出策略：Visible 可超出自身；Clip 裁剪；Scroll 自带滚动。
    enum class Overflow
    {
        Visible,
        Clip,
        Scroll,
    };
    virtual Overflow overflow() const { return Overflow::Visible; }
    bool clips_children() const { return overflow() != Overflow::Visible; }

    // 绘制/命中层次：高层的先命中、后绘制。
    enum class Layer
    {
        Content,
        Floating,
        Modal,
    };
    virtual Layer layer() const { return Layer::Content; }

    int z_index() const { return z_index_; }
    void set_z_index(int value) { z_index_ = value; }

    // 排序与命中（layer 升序 → z 升序；稳定）。
    static std::vector<Node*> sorted_paint_order(const std::vector<std::unique_ptr<Node>>& list)
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
    std::vector<Node*> paint_order() const { return sorted_paint_order(children()); }
    std::vector<Node*> overlay_paint_order() const { return sorted_paint_order(overlays()); }
    std::vector<Node*> layer_paint_order() const;

    virtual bool hit_testable() const { return true; }
    virtual bool focusable() const { return false; }
    // 打开模态/对话框时建议的初始焦点：默认无偏好。
    virtual Node* preferred_focus() { return nullptr; }

    // ── 语义：派生投影，不存第二份真值 ───────────────────────
    virtual WidgetRole semantics_role() const { return WidgetRole::Custom; }
    virtual std::string_view semantics_label() const { return {}; }
    virtual bool semantics_expanded() const { return false; }
    // 实现放在 LayoutNode（要读 Interaction 部件）。
    virtual Semantics semantics() const = 0;

    // ── 调试 ────────────────────────────────────────────────────────
    void debug() { debug_ = true; }
    void set_debug(bool enabled) { debug_ = enabled; mark_layout_dirty(); }
    bool is_debug() const { return debug_; }
    void debug_recursive()
    {
        debug();
        for_each_child([](Node& child) { child.debug_recursive(); });
    }
    void append_debug_tree(graphics::DrawList& list) const;
    void debug_rect(graphics::DrawList& list) const;

    // ── 布局契约（实现在 LayoutNode）──────────────────────────────────
    // 声明在 Node 上是为了让遍历代码无需向下转型即可调用。
    virtual glm::vec2 measure(glm::vec2 constraints) = 0;
    virtual void arrange(Rect slot) = 0;
    virtual void mark_layout_dirty() = 0;
    virtual void invalidate_measure() = 0;
    // 递归失效：主题/密度/字号切换后必须走这个。
    void invalidate_measure_recursive()
    {
        invalidate_measure();
        for_each_child([](Node& child) { child.invalidate_measure_recursive(); });
    }
    // 清除脏标记与「已通知祖先」标记（必须递归）。
    void clear_layout_dirty() { clear_layout_dirty_recursive(); }
    virtual void clear_layout_dirty_recursive() = 0;

protected:
    Rect global_;
    Rect local_;
    std::vector<std::unique_ptr<Node>> children_;
    Node* parent_ = nullptr;
    UIContext* context_ = nullptr;
    std::shared_ptr<const bool> alive_token_ = std::make_shared<const bool>(true);
    Display display_ = Display::Flow;
    bool visible_ = true;
    int z_index_ = 0;
    bool debug_ = false;
};
} // namespace xel::ui
