#pragma once

#include "ui/node/node.hpp"

#include "ui/geom/align.hpp"
#include "ui/geom/layout_metrics.hpp"
#include "ui/input/interaction.hpp"
#include "ui/layout/layout_params.hpp"
#include "ui/layout/measure_cache.hpp"
#include "ui/style/box_metrics.hpp"
#include "ui/style/visual_state.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>

namespace xel::ui
{
struct Theme;
struct BoxStyle;

// ── LayoutNode（设计 §4.2）：布局机制 + 4 个部件 ─────────────────────
//
// 相对 Node 增加的只有“布局意图与机制”：
//   layout_（意图）/ box（盒模型）/ input（交互状态）/ visual（样式过渡）/
//   measure_cache_（唯一缓存）/ 脏标记。
//
// Node 声明的纯虚契约在这里落地：measure / arrange / mark_layout_dirty /
// invalidate_measure / clear_layout_dirty_recursive / semantics。
//
// 使用者仍然写 `Widget`（见 widget.hpp 的别名），400+ 调用点零改动。
class LayoutNode : public Node
{
public:
    LayoutNode();
    LayoutNode(const LayoutNode&) = delete;
    LayoutNode& operator=(const LayoutNode&) = delete;
    ~LayoutNode() override;

    // ── 语义投影（UI-52）：状态一律从 Interaction 现读 ──────────────────
    Semantics semantics() const override
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
    // 禁用语义（UI-52）：以前 8 个控件各自持有一个 bool enabled_。
    bool is_enabled() const { return input.enabled; }
    virtual void set_enabled(bool enabled)
    {
        input.enabled = enabled;
        mark_layout_dirty();
    }
    bool enabled() const { return is_enabled(); }

    // ── 交互状态（由 UISystem 写入）──────────────────────────────────
    bool is_hovered() const { return input.hovered; }
    void set_hovered(bool hovered) { input.hovered = hovered; }
    bool is_focused() const { return input.focused; }
    void set_focused(bool focused) { input.focused = focused; }
    // 焦点可见性由 FocusPolicy 决定；样式解析与焦点环实现在 layout_node.cpp。
    bool focus_visible() const;
    void append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const;

    // ── 对齐与布局意图快照（UI-36 / UI-37）────────────────────────────
    void set_align(Align a)
    {
        LayoutSpec spec = layout_spec();
        spec.align = a;
        set_layout_spec(spec);
    }
    std::optional<Align> align_self() const
    {
        return layout_.has_align ? std::optional<Align>(layout_.align_self) : std::nullopt;
    }

    // 尺寸与对齐规格的不可变快照：布局意图的唯一数据形态。
    // 存储仍在 `layout_` 里，但**写入只经 set_layout_spec()**（UI-37）。
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
    LayoutSpec layout_spec() const
    {
        return {align_self(), layout_.flex_grow, layout_.size_percent, layout_.aspect_ratio,
                layout_.fill_x, layout_.fill_y, layout_.min_size, layout_.max_size,
                layout_.preferred};
    }
    // 布局字段的唯一写入者（UI-37）。
    void set_layout_spec(const LayoutSpec& spec)
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

    // ── 盒模型（UI-35）：显式调用即「锁住」，主题重刷不覆盖 ──────────────
    void set_padding(Edges edges)
    {
        box.padding = edges;
        box.padding_override = true;
        mark_layout_dirty();
    }
    void set_padding(float all) { set_padding(Edges{all, all, all, all}); }
    Edges padding() const { return box.padding; }
    bool padding_override() const { return box.padding_override; }
    void clear_padding_override() { box.padding_override = false; }
    // 主题套用入口（Panel / TabView 的 set_context 用）：显式设定过的内边距不覆盖。
    void apply_theme_padding(float all)
    {
        if (box.padding_override) return;
        box.padding = {all, all, all, all};
        mark_layout_dirty();
    }
    void set_margin(Edges edges)
    {
        box.margin = edges;
        mark_layout_dirty();
    }
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

    // ── 尺寸意图 ───────────────────────────────────────────────────
    void set_flex_grow(float grow)
    {
        LayoutSpec spec = layout_spec();
        spec.flex_grow = grow;
        set_layout_spec(spec);
    }
    float flex_grow() const { return layout_.flex_grow; }

    // 绝对定位（Positioned）：按 (x, y) 相对父内容区定位；容器不计入其尺寸。
    void set_absolute(bool value);
    bool absolute() const { return display() == Display::Positioned; }
    // 相对偏移与全局位置分离：arrange 会改写 local_.pos（全局）。
    void set_absolute_pos(glm::vec2 pos)
    {
        layout_.absolute_pos = pos;
        set_display(Display::Positioned);
    }
    glm::vec2 absolute_pos() const { return layout_.absolute_pos; }

    void set_min_size(glm::vec2 size)
    {
        LayoutSpec spec = layout_spec();
        spec.min_size = size;
        set_layout_spec(spec);
    }
    void set_max_size(glm::vec2 size)
    {
        LayoutSpec spec = layout_spec();
        spec.max_size = size;
        set_layout_spec(spec);
    }
    glm::vec2 min_size() const { return layout_.min_size; }
    glm::vec2 max_size() const { return layout_.max_size; }
    glm::vec2 clamp_size(glm::vec2 size) const
    {
        return glm::clamp(size, layout_.min_size, layout_.max_size);
    }

    // 叶子的自然尺寸（UI-32）：构造期确定，与 arrange 的几何分离。
    void set_preferred_size(glm::vec2 size)
    {
        LayoutSpec spec = layout_spec();
        spec.preferred = size;
        set_layout_spec(spec);
    }
    glm::vec2 preferred_size() const { return layout_.preferred; }

    // 外部显式指定尺寸：几何与自然尺寸一起改（调试面板「用户改 Size」用）。
    void request_size(glm::vec2 size)
    {
        set_preferred_size(size);
        set_size(size);
    }

    void set_size_percent(glm::vec2 pct)
    {
        LayoutSpec spec = layout_spec();
        spec.size_percent = pct;
        set_layout_spec(spec);
    }
    void set_size_percent(float x, float y) { set_size_percent({x, y}); }
    glm::vec2 size_percent() const { return layout_.size_percent; }

    // Fill 尺寸模式：沿该轴吃满父级内容区（语义显式，等价 pct=1）。
    void set_fill(bool x, bool y)
    {
        LayoutSpec spec = layout_spec();
        spec.fill_x = x;
        spec.fill_y = y;
        set_layout_spec(spec);
    }
    bool fill_x() const { return layout_.fill_x; }
    bool fill_y() const { return layout_.fill_y; }
    void set_aspect_ratio(float ratio)
    {
        LayoutSpec spec = layout_spec();
        spec.aspect_ratio = ratio;
        set_layout_spec(spec);
    }
    float aspect_ratio() const { return layout_.aspect_ratio; }

    // ── 子项访问（UI-56）：隐藏基类同名函数，返回 LayoutNode* ───────────
    // 本项目所有 Node 都是 LayoutNode（见文件末尾的 as_layout 说明），所以这里
    // 用协变返回类型提供布局视图：容器代码写 child_at(i)->min_size() 不需要转型。
    LayoutNode* child_at(std::size_t index) const
    {
        return static_cast<LayoutNode*>(Node::child_at(index));
    }
    LayoutNode* child(std::size_t index) const { return child_at(index); }
    // 容器的子项遍历（布局视图）：需要布局属性的地方用它，而不是 for_each_child + 下行。
    template <class F>
    void for_each_layout_child(F&& f) const
    {
        for (std::size_t i = 0; i < child_count(); ++i) f(*child_at(i));
    }

    // ── 容器测量子项的统一入口（唯一）────────────────────────────────
    // constraints 是父级内容区可用尺寸（已扣 padding、**未扣 margin**）；
    // 结果带 (constraints, 布局轮次) 缓存。接受 Node&：遍历代码无需向下转型。
    static glm::vec2 measure_child(Node& child, glm::vec2 constraints);
    static glm::vec2 measure_child(LayoutNode& child, glm::vec2 constraints);

    // ── 单控件圆角覆盖 ───────────────────────────────────────────────
    void set_corner_radius(float radius) { radius_override_ = radius; }
    void clear_corner_radius() { radius_override_.reset(); }
    float corner_radius(float fallback) const { return radius_override_.value_or(fallback); }

    // 文字基线相对控件顶边的偏移（UI-36）。
    virtual float baseline_offset_for(float h) const { return h * 0.5f; }
    float baseline_offset() const { return baseline_offset_for(height()); }

    // Tab 顺序：>0 显式顺序（小的先）；0 按树序；<0 不参与 Tab。
    void set_tab_order(int order) { tab_order_ = order; }
    int tab_order() const { return tab_order_; }

    // ── 事件钩子（Q1 会把它们换成能力接口）──────────────────────────
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

    // ── 容器/尺寸策略（派生类按需覆写）────────────────────────────
    // 固定尺寸控件：忽略 constraints，尺寸由 min_size/max_size 决定（Table/FileBrowser）。
    virtual bool fixed_size() const { return false; }
    // 铺满父容器：不参与测量协商（模态遮罩这类）。
    virtual bool fills_parent() const { return false; }
    // 布局完成后的钩子（Window 用它摆内部 chrome）。
    virtual void layout() {}

    // ── 每帧钩子：推进过渡，再交给控件自己的动画钩子（NVI）────────────
    void update(float dt)
    {
        advance_transition(dt);
        on_update(dt);
    }

    // 状态样式：按角色解析 + 过渡。替代直接 ui_theme().style(...)。
    BoxStyle visual_style(StyleRole role, ControlState state) const;
    bool transition_active() const;

    // ── 布局契约（Node 声明的纯虚在这里落地）─────────────────────────
    void mark_layout_dirty() override;
    void invalidate_measure() override { measure_cache_.invalidate(); }
    void clear_layout_dirty_recursive() override;
    // 本轮是否请求过重排（调试面板与断言用）。
    bool layout_dirty() const { return layout_dirty_; }

    // 测量缓存入口（UI-51 统一）：布局级命中 + 派生输入命中。
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
    bool content_overflows() const
    {
        constexpr float kEps = 0.5f;
        return content_extent_.x > global_.size.x + kEps || content_extent_.y > global_.size.y + kEps;
    }

protected:
    virtual void on_update(float /*dt*/) {}
    // 构造期的结构性默认内边距（Vbox/Hbox 的 4px）：不锁定，主题仍可接管。
    void set_default_padding(float all) { box.padding = {all, all, all, all}; }
    // 容器 arrange 时记录内容实际占用尺寸。
    void set_content_extent(glm::vec2 extent) { content_extent_ = extent; }

    // ── 四个部件（UI-48）──────────────────────────────────────────
    LayoutParams layout_;   // 布局意图（写入只经 set_layout_spec）
    BoxMetrics   box;       // padding / padding_override / margin
    Interaction  input;     // 状态唯一所有者
    VisualState  visual;    // 样式过渡（104 B 内嵌，无堆）

    // 测量缓存（UI-51）：一套到底；protected 让派生类填派生输入。
    mutable MeasureCache measure_cache_{};

    bool layout_dirty_ = true;
    int tab_order_ = 0;
    glm::vec2 content_extent_{0.0f, 0.0f};
    // 本轮是否已把“失效”告知祖先链（UI-34 早退）。
    bool notified_ = false;

private:
    float transition_duration() const;
    BoxStyle current_transition_style(StyleRole role) const;
    void advance_transition(float dt);

    std::optional<float> radius_override_;
};

// ── Node → LayoutNode 的合法下行（UI-56）────────────────────────────
// 本项目里**所有** Node 都是 LayoutNode（Node 只作为树的元素类型存在），
// 因此这里用 static_cast 是安全的；写成函数是为了让下行点显式、可搜。
inline LayoutNode&       as_layout(Node& node)       { return static_cast<LayoutNode&>(node); }
inline const LayoutNode& as_layout(const Node& node) { return static_cast<const LayoutNode&>(node); }
inline LayoutNode*       as_layout(Node* node)       { return static_cast<LayoutNode*>(node); }
inline const LayoutNode* as_layout(const Node* node) { return static_cast<const LayoutNode*>(node); }
} // namespace xel::ui
