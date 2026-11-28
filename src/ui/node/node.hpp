#pragma once

#include "events/key_code.hpp"
#include "graphics/drawable.hpp"
#include "ui/geom/align.hpp"
#include "ui/geom/layout_metrics.hpp"
#include "ui/geom/rect.hpp"
#include "ui/input/interaction.hpp"
#include "ui/layout/layout_params.hpp" // Display / LayoutParams
#include "ui/layout/measure_cache.hpp"
#include "ui/semantics/semantics.hpp"
#include "ui/style/box_metrics.hpp"
#include "ui/style/style_types.hpp"
#include "ui/style/visual_state.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

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

// ── Node：整棵 UI 树的唯一元素类型（`Widget` 是它的别名）─────────────
//
// 能力按层分组，每组只暴露一个入口：
//   几何      —— 只有一份 bounds_（全局坐标），写只经 set_geometry
//   树        —— children_ 一份存储，挂载只经 adopt
//   可见性    —— visible_ + Display（Hidden 是 display 的一个取值）
//   层叠      —— Layer + z_index → paint_order / layer_paint_order
//   语义      —— 派生投影（semantics() 现读 Interaction，不存副本）
//   交互状态  —— Interaction（input）唯一所有者
//   布局意图  —— LayoutSpec 快照；写入只经 set_layout_spec
//   盒模型    —— BoxMetrics（padding / margin）
//   测量      —— MeasureCache + 脏标记；measure_child 是容器测子项的唯一入口
//   事件      —— 17 个 on_* 钩子
//   调试      —— debug 标记与包围盒绘制
//
// 布局契约有默认实现（`measure = clamp(preferred_size)`、`arrange = set_geometry(slot)`），
// 即“叶子”行为；语义不同的控件自行覆写。
class Node : public graphics::Drawable
{
public:
    Node();
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    ~Node() override;

    graphics::TextureSlot texture_slot() const override { return graphics::TextureSlot::Texture; }

    // ════════════════════════════════════════════════════════════════
    // 几何：读随处可读，写只经 set_geometry（K3）
    // ════════════════════════════════════════════════════════════════
    // 坐标只有一份存储，且一律是全局坐标：父容器 arrange 子项时传的就是绝对矩形，
    // 没有「局部 → 全局」的变换需要维护。
    float x()      const { return bounds_.pos.x; }
    float y()      const { return bounds_.pos.y; }
    float width()  const { return bounds_.size.x; }
    float height() const { return bounds_.size.y; }
    Rect bounds()  const { return bounds_; }

    // 外部请求（会标脏；布局期间无效，见 mark_layout_dirty）。
    void set_pos(float x, float y) { bounds_.pos = {x, y}; mark_layout_dirty(); }
    void set_size(float width, float height) { bounds_.size = {width, height}; mark_layout_dirty(); }
    void set_pos(glm::vec2 pos) { bounds_.pos = pos; mark_layout_dirty(); }
    void set_size(glm::vec2 size) { bounds_.size = size; mark_layout_dirty(); }

    // 布局写入入口（只能在 arrange 内用）：只改几何，不触发脏标记。
    void set_geometry(Rect slot) { bounds_ = slot; }

    // ════════════════════════════════════════════════════════════════
    // 树：存储私有，遍历只有 for_each_child 一条路（K2）
    // ════════════════════════════════════════════════════════════════
    // 任何 Node 都能容纳子节点（add 默认转发 adopt）；需要改写挂载目标的复合控件
    // （Window/Section/TabView…）仍可覆写 add()。
    //
    // 只有一条子项列表：“浮在上面”是子项自己的 Layer / z_index，不是第二份存储。
    // 因此浮层（弹层/模态/提示）就是所属宿主（通常是 UIRoot）的普通子项：
    // 宿主不裁剪它们，layer_paint_order 负责绘制/命中顺序，递归工具自动覆盖。
    virtual void add(std::unique_ptr<Node> child) { adopt(std::move(child)); }
    const std::vector<std::unique_ptr<Node>>& children() const { return children_; }

    Node* parent() const { return parent_; }
    void set_parent(Node* parent) { parent_ = parent; }

    std::size_t child_count() const { return children_.size(); }
    Node* child_at(std::size_t index) const
    {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

    // 唯一合法的子项挂载入口：设 parent / context 并加入 children_。
    void adopt(std::unique_ptr<Node> child);
    // 清空子项（销毁）。公开：与 add()/adopt() 同级（容器内容整体重建）。
    void clear_children();
    // 把第 index 个子项的所有权交出去（不销毁）：事件派发期间可先挂到待销毁队列。
    std::unique_ptr<Node> detach_child(std::size_t index);
    // 替换第 index 个子项（TabView 换页用）。
    void replace_child(std::size_t index, std::unique_ptr<Node> child);
    // 把已有子项移到末尾（插入序置顶，不改 z_index）。
    void raise_child(Node* child);

    // 树的唯一遍历入口。
    template <class F>
    void for_each_child(F&& f) const
    {
        for (const auto& child : children_) f(*child);
    }

    // 生命周期令牌：容器可能直接销毁子控件，UISystem 用 weak_ptr 判断悬垂。
    const std::shared_ptr<const bool>& alive_token() const { return alive_token_; }

    // ════════════════════════════════════════════════════════════════
    // 上下文 / 主题
    // ════════════════════════════════════════════════════════════════
    virtual void set_context(UIContext* context);
    UIContext* context() const
    {
        if (context_) return context_;
        return parent_ ? parent_->context() : nullptr;
    }
    const Theme& ui_theme() const;
    graphics::FontAtlas* font_atlas() const;

    // ════════════════════════════════════════════════════════════════
    // 可见性 / 层叠 / 绘制顺序
    // ════════════════════════════════════════════════════════════════
    // 定义在 layout_params.hpp；别名保持 `Widget::Display::Flow` 写法不变。
    using Display = ::xel::ui::Display;

    Display display() const { return !visible_ ? Display::Hidden : display_; }
    void set_display(Display value);
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
    std::vector<Node*> paint_order() const;
    std::vector<Node*> layer_paint_order() const;

    virtual bool hit_testable() const { return true; }
    virtual bool focusable() const { return false; }
    // 打开模态/对话框时建议的初始焦点：默认无偏好。
    virtual Node* preferred_focus() { return nullptr; }

    // ════════════════════════════════════════════════════════════════
    // 语义：派生投影，不存第二份真值
    // ════════════════════════════════════════════════════════════════
    virtual WidgetRole semantics_role() const { return WidgetRole::Custom; }
    virtual std::string_view semantics_label() const { return {}; }
    virtual bool semantics_expanded() const { return false; }
    // 状态从 Interaction 现读。
    Semantics semantics() const;

    // ════════════════════════════════════════════════════════════════
    // 交互状态（由 UISystem 写入）
    // ════════════════════════════════════════════════════════════════
    bool is_enabled() const { return input.enabled; }
    virtual void set_enabled(bool enabled);
    bool is_hovered() const { return input.hovered; }
    void set_hovered(bool hovered) { input.hovered = hovered; }
    bool is_focused() const { return input.focused; }
    void set_focused(bool focused) { input.focused = focused; }
    // 焦点可见性由 FocusPolicy 决定（实现与焦点环绘制在 node.cpp）。
    bool focus_visible() const;
    void append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const;

    // ════════════════════════════════════════════════════════════════
    // 布局意图：LayoutSpec 是不可变快照，写入只经 set_layout_spec
    // ════════════════════════════════════════════════════════════════
    struct LayoutSpec
    {
        std::optional<Align> align;
        float flex_grow = 0.0f;
        glm::vec2 size_percent{-1.0f, -1.0f};
        float aspect_ratio = 0.0f;
        bool fill_x = false;
        bool fill_y = false;
        glm::vec2 min_size{0.0f, 0.0f};
        glm::vec2 max_size{0.0f, 0.0f};
        glm::vec2 preferred{0.0f, 0.0f};
    };

    LayoutSpec layout_spec() const;
    void set_layout_spec(const LayoutSpec& spec);

    // 对齐
    void set_align(Align a);
    std::optional<Align> align_self() const
    {
        return layout_.has_align ? std::optional<Align>(layout_.align_self) : std::nullopt;
    }

    // 尺寸边界与自然尺寸
    void set_min_size(glm::vec2 size);
    void set_max_size(glm::vec2 size);
    glm::vec2 min_size() const { return layout_.min_size; }
    glm::vec2 max_size() const { return layout_.max_size; }
    glm::vec2 clamp_size(glm::vec2 size) const
    {
        return glm::clamp(size, layout_.min_size, layout_.max_size);
    }
    // 叶子的自然尺寸：构造期确定，与 arrange 的几何分离。
    void set_preferred_size(glm::vec2 size);
    glm::vec2 preferred_size() const { return layout_.preferred; }
    // 外部显式指定尺寸：几何与自然尺寸一起改（调试面板「用户改 Size」用）。
    void request_size(glm::vec2 size);

    // 相对尺寸策略（优先级：fill > percent > preferred）
    void set_fill(bool x, bool y);
    bool fill_x() const { return layout_.fill_x; }
    bool fill_y() const { return layout_.fill_y; }
    void set_size_percent(glm::vec2 pct);
    void set_size_percent(float x, float y) { set_size_percent({x, y}); }
    glm::vec2 size_percent() const { return layout_.size_percent; }
    void set_aspect_ratio(float ratio);
    float aspect_ratio() const { return layout_.aspect_ratio; }
    void set_flex_grow(float grow);
    float flex_grow() const { return layout_.flex_grow; }

    // 绝对定位（Positioned）：按 (x, y) 相对父内容区定位；容器不计入其尺寸。
    void set_absolute(bool value);
    bool absolute() const { return display() == Display::Positioned; }
    // 相对偏移与几何位置分离：arrange 会改写 bounds_.pos。
    void set_absolute_pos(glm::vec2 pos);
    glm::vec2 absolute_pos() const { return layout_.absolute_pos; }

    // ════════════════════════════════════════════════════════════════
    // 盒模型：显式调用即「锁住」，主题重刷不覆盖
    // ════════════════════════════════════════════════════════════════
    void set_padding(Edges edges);
    void set_padding(float all) { set_padding(Edges{all, all, all, all}); }
    Edges padding() const { return box.padding; }
    bool padding_override() const { return box.padding_override; }
    // 主题套用入口（Panel / TabView 的 set_context 用）：显式设定过的内边距不覆盖。
    void apply_theme_padding(float all);
    void set_margin(Edges edges);
    void set_margin(float all) { set_margin(Edges{all, all, all, all}); }
    Edges margin() const { return box.margin; }

    float padding_left()   const { return box.padding.left; }
    float padding_right()  const { return box.padding.right; }
    float padding_top()    const { return box.padding.top; }
    float padding_bottom() const { return box.padding.bottom; }
    float margin_left()    const { return box.margin.left; }
    float margin_right()   const { return box.margin.right; }
    float margin_top()     const { return box.margin.top; }
    float margin_bottom()  const { return box.margin.bottom; }

    // ════════════════════════════════════════════════════════════════
    // 测量：契约 + 缓存 + 脏标记
    // ════════════════════════════════════════════════════════════════
    // 默认即叶子行为；语义不同的控件自行覆写（判据：只是
    // 「preferred → clamp / slot → set_geometry」就不必写）。
    virtual glm::vec2 measure(glm::vec2 /*constraints*/)
    {
        return clamp_size(preferred_size());
    }
    virtual void arrange(Rect slot) { set_geometry(slot); }

    // 容器测量子项的唯一入口：constraints 是父级内容区可用尺寸
    // （已扣 padding、**未扣 margin**）；结果带 (constraints, 布局轮次) 缓存。
    static glm::vec2 measure_child(Node& child, glm::vec2 constraints);

    virtual void mark_layout_dirty();
    virtual void invalidate_measure() { measure_cache_.invalidate(); }
    // 递归失效：主题/密度/字号切换后必须走这个。
    void invalidate_measure_recursive();
    // 清除脏标记与「已通知祖先」标记（必须递归）。
    void clear_layout_dirty() { clear_layout_dirty_recursive(); }
    virtual void clear_layout_dirty_recursive();
    // 本轮是否请求过重排。
    bool layout_dirty() const { return layout_dirty_; }

    // 测量缓存入口：布局级命中 + 派生输入命中。
    bool measure_cache_hit(glm::vec2 constraints) const;
    bool measure_cache_hit(const MeasureKey& probe) const { return measure_cache_.hit(probe); }
    glm::vec2 measure_cached() const { return measure_cache_.result; }
    void store_measure(glm::vec2 constraints, glm::vec2 result);
    void store_measure(const MeasureKey& probe, glm::vec2 result)
    {
        measure_cache_.store(probe, result);
    }
    bool measure_valid() const { return measure_cache_.valid; }
    // 本轮已统计过（测过或按规则跳过）：清除“已通知祖先”标记。
    void mark_measured()
    {
        measure_cache_.valid = true;
        notified_ = false;
    }

    // 内容实际占用尺寸（容器在 arrange 里写入）；用于判断是否溢出。
    glm::vec2 content_extent() const { return content_extent_; }
    bool content_overflows() const;

    // ════════════════════════════════════════════════════════════════
    // 事件钩子（17 个）
    // ════════════════════════════════════════════════════════════════
    virtual bool on_mouse_move(glm::vec2 pos) { (void)pos; return false; }
    virtual void on_hover(glm::vec2 pos) { (void)pos; }
    virtual void on_mouse_leave() {}
    virtual void on_mouse_enter() {}
    virtual bool on_mouse_down(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_mouse_up(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_right_click(glm::vec2 pos) { (void)pos; return false; }
    virtual bool on_double_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_long_press(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_scroll(glm::vec2 pos, glm::vec2 delta) { (void)pos; (void)delta; return false; }
    virtual bool on_key_down(event::KeyCode key, event::Modifier mods) { (void)key; (void)mods; return false; }
    virtual bool on_key_up(event::KeyCode key, event::Modifier mods) { (void)key; (void)mods; return false; }
    virtual bool on_char(char32_t codepoint) { (void)codepoint; return false; }
    virtual void on_focus_gained() { input.focused = true; }
    virtual void on_focus_lost() { input.focused = false; }

    // ════════════════════════════════════════════════════════════════
    // 尺寸策略 / 生命周期 / 样式
    // ════════════════════════════════════════════════════════════════
    // 固定尺寸控件：忽略 constraints，尺寸由 min_size/max_size 决定（Table/FileBrowser）。
    virtual bool fixed_size() const { return false; }
    // 铺满父容器：不参与测量协商（模态遮罩这类）。
    virtual bool fills_parent() const { return false; }
    // 布局完成后的钩子（Window 用它摆内部 chrome）。
    virtual void layout() {}

    // 每帧钩子：推进过渡，再交给控件自己的动画钩子（NVI）。
    void update(float dt)
    {
        advance_transition(dt);
        on_update(dt);
    }

    // 状态样式：按角色解析 + 过渡。替代直接 ui_theme().style(...)。
    BoxStyle visual_style(StyleRole role, ControlState state) const;
    bool transition_active() const;

    // 单控件圆角覆盖（未设置时用主题的）。
    void set_corner_radius(float radius) { radius_override_ = radius; }
    float corner_radius(float fallback) const { return radius_override_.value_or(fallback); }

    // 文字基线相对控件顶边的偏移。
    virtual float baseline_offset_for(float h) const { return h * 0.5f; }
    float baseline_offset() const { return baseline_offset_for(height()); }

    // Tab 顺序：>0 显式顺序（小的先）；0 按树序；<0 不参与 Tab。
    void set_tab_order(int order) { tab_order_ = order; }
    int tab_order() const { return tab_order_; }

    // ════════════════════════════════════════════════════════════════
    // 调试
    // ════════════════════════════════════════════════════════════════
    void debug() { debug_ = true; }
    void set_debug(bool enabled) { debug_ = enabled; mark_layout_dirty(); }
    bool is_debug() const { return debug_; }
    void debug_recursive();
    void append_debug_tree(graphics::DrawList& list) const;
    void debug_rect(graphics::DrawList& list) const;

protected:
    virtual void on_update(float /*dt*/) {}
    // 构造期的结构性默认内边距（Vbox/Hbox 的 4px）：不锁定，主题仍可接管。
    void set_default_padding(float all) { box.padding = {all, all, all, all}; }
    // 容器 arrange 时记录内容实际占用尺寸。
    void set_content_extent(glm::vec2 extent) { content_extent_ = extent; }

    // ── 节点身份 / 树 ───────────
    Rect bounds_;
    std::vector<std::unique_ptr<Node>> children_;
    Node* parent_ = nullptr;
    UIContext* context_ = nullptr;
    std::shared_ptr<const bool> alive_token_ = std::make_shared<const bool>(true);
    Display display_ = Display::Flow;
    bool visible_ = true;
    int z_index_ = 0;
    bool debug_ = false;

    // ── 四个部件 ───────────────
    LayoutParams layout_;   // 布局意图（写入只经 set_layout_spec）
    BoxMetrics   box;       // padding / padding_override / margin
    Interaction  input;     // 状态唯一所有者
    VisualState  visual;    // 样式过渡（104 B 内嵌，无堆）

    // 测量缓存：一套到底；protected 让派生类填派生输入。
    mutable MeasureCache measure_cache_{};

    bool layout_dirty_ = true;
    int tab_order_ = 0;
    glm::vec2 content_extent_{0.0f, 0.0f};
    // 本轮是否已把“失效”告知祖先链（早退）。
    bool notified_ = false;

private:
    float transition_duration() const;
    BoxStyle current_transition_style(StyleRole role) const;
    void advance_transition(float dt);

    std::optional<float> radius_override_;
};
} // namespace xel::ui
