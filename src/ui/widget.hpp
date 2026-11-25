#pragma once

#include "graphics/drawable.hpp"
#include "ui/rect.hpp"
#include "ui/align.hpp"
#include "ui/layout_metrics.hpp"
#include "ui/style_types.hpp"
#include "events/key_code.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace xel::graphics
{
class FontAtlas;
}

namespace xel::ui
{
struct UIContext;
struct Theme;
struct BoxStyle;
struct StyleTransition;

// 布局阶段守卫：UIRoot::layout() 期间生效。
// 目的：把“布局写入”（arrange 里 set_size/set_geometry）与“失效请求”（外部改尺寸）区分开。
// 实现见 widget.cpp；用全局计数器是因为 mark_layout_dirty() 是头文件内联函数。
bool in_layout_pass();

class LayoutPassScope
{
public:
    LayoutPassScope();
    ~LayoutPassScope();
    LayoutPassScope(const LayoutPassScope&) = delete;
    LayoutPassScope& operator=(const LayoutPassScope&) = delete;
};

struct Edges
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

class Widget : public graphics::Drawable
{
public:
    Widget();
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    virtual ~Widget();

    graphics::TextureSlot texture_slot() const override { return graphics::TextureSlot::Texture; }

    virtual float x()       const { return local_.pos.x; }
    virtual float y()       const { return local_.pos.y; }
    virtual float width()   const { return local_.size.x; }
    virtual float height()  const { return local_.size.y; }

    Rect local_bounds()  const { return local_; }
    Rect global_bounds() const { return global_; }

    virtual void set_pos(float x, float y) { local_.pos = {x, y}; mark_layout_dirty(); }
    virtual void set_size(float width, float height) { local_.size = {width, height}; mark_layout_dirty(); }
    virtual void set_pos(glm::vec2 pos) { local_.pos = pos; mark_layout_dirty(); }
    virtual void set_size(glm::vec2 size) { local_.size = size; mark_layout_dirty(); }
    virtual void set_global_bounds(Rect bounds) { global_ = bounds; }

    // 布局写入入口（只能在 arrange 内用）：只改几何，不触发脏标记。
    // slot 是全局矩形；local_ 与 global_ 同步，保证 local_bounds() == 最近一次 arrange 的 slot。
    // 没它的话，arrange 里调 set_pos/set_size 会把整棵树重新弄脏（历史上因此各容器不敢写 local_）。
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

    virtual void add(std::unique_ptr<Widget> child)
    {
        (void)child;
        throw std::logic_error("Widget::add is not supported by this widget");
    }
    virtual const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    // overlay 是顶层绘制/命中层：root 先绘制普通子项，再绘制 overlay；命中测试反向进行。
    virtual const std::vector<std::unique_ptr<Widget>>& overlays() const { static const std::vector<std::unique_ptr<Widget>> empty; return empty; }
    virtual Widget* parent() const { return parent_; }
    void set_parent(Widget* parent) { parent_ = parent; }

    // 唯一合法的子项挂载入口：设 parent / context 并加入 children_。
    // 派生类不要再直接 children_.push_back —— 漏设 parent 会让事件冒泡链断掉。
    void adopt(std::unique_ptr<Widget> child)
    {
        child->set_parent(this);
        child->set_context(context());
        children_.push_back(std::move(child));
        mark_layout_dirty();
    }

    // 把已有子项移到 children_ 末尾（插入序置顶）：
    // layer_paint_order 在同 layer 同 z 时按加入顺序排序，因此移动位置即可“提到最上”。
    // 不修改任何控件的 z_index，不会破坏调用方显式设置的层叠关系。
    void raise_child(Widget* child)
    {
        if (!child) return;
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->get() != child) continue;
            if (std::next(it) == children_.end()) return; // 已在末尾
            auto owned = std::move(*it);
            children_.erase(it);
            children_.push_back(std::move(owned));
            mark_layout_dirty();
            return;
        }
    }

    // 生命周期令牌：容器可能直接销毁子控件（如 clear_children），
    // UISystem 用 weak_ptr 判断保存的指针是否仍然有效，避免悬垂解引用。
    const std::shared_ptr<const bool>& alive_token() const { return alive_token_; }

    // UIContext 沿 parent 链查找；root 持有 context，子控件不重复拥有资源。
    virtual void set_context(UIContext* context)
    {
        context_ = context;
        for (const auto& child : children()) child->set_context(context);
        for (const auto& overlay : overlays()) overlay->set_context(context);
    }
    UIContext* context() const
    {
        if (context_) return context_;
        return parent_ ? parent_->context() : nullptr;
    }
    const Theme& ui_theme() const;
    graphics::FontAtlas* font_atlas() const;
    // 容器是否把子控件裁剪在自身范围内（命中测试与渲染共用同一语义）
    virtual bool clips_children() const { return false; }

    bool visible() const { return visible_; }
    bool is_hovered() const { return hovered_; }
    void set_hovered(bool hovered) { hovered_ = hovered; }

    // 焦点由基类持有：控件不再各自保存 focused_，样式解析与焦点环统一读这里。
    bool is_focused() const { return focused_; }
    void set_focused(bool focused) { focused_ = focused; }
    // 焦点可见性由 FocusPolicy 决定；样式解析与焦点环实现在 widget.cpp。
    bool focus_visible() const;
    void append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const;
    void set_visible(bool visible) { visible_ = visible; mark_layout_dirty(); }
    virtual void debug() { debug_ = true; }
    void set_debug(bool enabled) { debug_ = enabled; mark_layout_dirty(); }
    bool is_debug() const { return debug_; }
    void debug_recursive()
    {
        debug();
        for (const auto& child : children()) child->debug_recursive();
    }
    void append_debug_tree(graphics::DrawList& list) const;
    void debug_rect(graphics::DrawList& list) const;
    virtual void layout() {}
    // 布局契约：constraints.x 是父容器给的可用宽度，返回值宽度不得超过它
    // （min_size 强制除外）。违反这条会让并列控件互相挤压。
    virtual glm::vec2 measure(glm::vec2 constraints) = 0;
    virtual void arrange(Rect slot) = 0;

    // 固定尺寸控件：忽略 constraints，尺寸由 min_size/max_size 决定。
    // 目的是把「Table/FileBrowser 这类吃满父宽」的行为从隐式约定变成显式声明。
    virtual bool fixed_size() const { return false; }

    // 铺满父容器：不参与测量协商（模态遮罩这类）。
    virtual bool fills_parent() const { return false; }

    // 绘制/命中层次：高层的先命中、后绘制。overlay 机制处理了大部分场景，
    // 这里只把「浮层/模态」这层语义显式化，便于后续窗口管理。
    enum class Layer
    {
        Content,
        Floating,
        Modal,
    };
    virtual Layer layer() const { return Layer::Content; }
    void set_align(Align a) { align_self_ = a; has_align_ = true; mark_layout_dirty(); }
    std::optional<Align> align_self() const { return has_align_ ? std::optional<Align>(align_self_) : std::nullopt; }

    // 内边距：显式调用即「锁住」该值，主题重刷（set_context / UIRoot::set_theme）不再覆盖。
    // 容器把主题值套进来时走 apply_theme_padding()，那条路径会跳过已锁定的控件。
    void    set_padding(Edges edges)    { padding_ = edges; padding_override_ = true; mark_layout_dirty(); }
    void    set_padding(float all)      { set_padding(Edges{all, all, all, all}); }
    Edges   padding() const             { return padding_; }
    bool    padding_override() const    { return padding_override_; }
    // 交还给主题：之后 apply_theme_padding() 才会重新生效。
    void    clear_padding_override()    { padding_override_ = false; }
    // 主题套用入口（Panel / TabView 的 set_context 用）：显式设定过的内边距不覆盖。
    void    apply_theme_padding(float all)
    {
        if (padding_override_) return;
        padding_ = {all, all, all, all};
        mark_layout_dirty();
    }
    void    set_margin(Edges edges)     { margin_ = edges; mark_layout_dirty(); }
    void    set_margin(float all)       { margin_ = {all, all, all, all}; mark_layout_dirty(); }
    Edges   margin() const              { return margin_; }

    void    set_flex_grow(float grow)   { flex_grow_ = grow < 0.0f ? 0.0f : grow; mark_layout_dirty(); }
    float   flex_grow() const           { return flex_grow_; }

    // 绝对定位：脱离流式布局，按 (x, y) 相对父内容区定位；容器不计入其尺寸。
    void set_absolute(bool value)
    {
        if (value && !absolute_) absolute_pos_ = local_.pos; // 兼容：从当前位置开始
        absolute_ = value;
        mark_layout_dirty();
    }
    bool absolute() const { return absolute_; }

    // 相对偏移与全局位置分离：arrange 会改写 local_.pos（全局），
    // 若 absolute 子项读 x()/y() 会每帧累加父偏移。
    void set_absolute_pos(glm::vec2 pos)
    {
        absolute_pos_ = pos;
        absolute_ = true;
        mark_layout_dirty();
    }
    glm::vec2 absolute_pos() const { return absolute_pos_; }

    // 同层绘制/命中顺序键：数值大的画在上面、命中优先。默认 0（按加入顺序）。
    void set_z_index(int value) { z_index_ = value; }
    int z_index() const { return z_index_; }

    // 按 z-index 稳定升序排列的一组控件；没有非零 z 时保持原顺序（避免每帧排序）。
    static std::vector<Widget*> sorted_paint_order(const std::vector<std::unique_ptr<Widget>>& list)
    {
        std::vector<Widget*> order;
        order.reserve(list.size());
        for (const auto& item : list) order.push_back(item.get());
        if (std::any_of(order.begin(), order.end(), [](Widget* w) { return w->z_index() != 0; })) {
            std::stable_sort(order.begin(), order.end(),
                [](Widget* a, Widget* b) { return a->z_index() < b->z_index(); });
        }
        return order;
    }
    std::vector<Widget*> paint_order() const { return sorted_paint_order(children()); }
    std::vector<Widget*> overlay_paint_order() const { return sorted_paint_order(overlays()); }

    // 跨 children_ 与 overlays_ 的唯一排序入口：layer 升序 → z 升序（稳定，同值按加入顺序）。
    // 绘制正序、命中反序、Tab 正序都从这里派生；overlay 不再是一等顺序来源。
    std::vector<Widget*> layer_paint_order() const
    {
        std::vector<Widget*> order;
        order.reserve(children().size() + overlays().size());
        for (const auto& child : children()) order.push_back(child.get());
        for (const auto& overlay : overlays()) order.push_back(overlay.get());
        std::stable_sort(order.begin(), order.end(), [](Widget* a, Widget* b) {
            if (a->layer() != b->layer()) return a->layer() < b->layer();
            return a->z_index() < b->z_index();
        });
        return order;
    }
    void set_min_size(glm::vec2 size) { min_size_ = size; mark_layout_dirty(); }
    void set_max_size(glm::vec2 size) { max_size_ = size; mark_layout_dirty(); }
    glm::vec2 min_size() const { return min_size_; }
    glm::vec2 max_size() const { return max_size_; }
    glm::vec2 clamp_size(glm::vec2 size) const { return glm::clamp(size, min_size_, max_size_); }

    // 尺寸规格：每轴占父级可用空间的比例（>0 启用）与宽高比（>0 启用）。
    void set_size_percent(glm::vec2 pct) { size_percent_ = pct; mark_layout_dirty(); }
    void set_size_percent(float x, float y) { set_size_percent({x, y}); }
    glm::vec2 size_percent() const { return size_percent_; }

    // Fill 尺寸模式：沿该轴吃满父级内容区。
    // 与 size_percent=1 的几何结果相同，但语义显式：
    //   - measure_child 在扣完 margin 后直接用可用尺寸（不再依赖“返回 constraints”的骗法）；
    //   - 父级据此知道“这个子项不该把我的自然尺寸撞大”（预算在 UI-32 统一）。
    void set_fill(bool x, bool y)
    {
        fill_x_ = x;
        fill_y_ = y;
        mark_layout_dirty();
    }
    bool fill_x() const { return fill_x_; }
    bool fill_y() const { return fill_y_; }
    void set_aspect_ratio(float ratio) { aspect_ratio_ = ratio > 0.0f ? ratio : 0.0f; mark_layout_dirty(); }
    float aspect_ratio() const { return aspect_ratio_; }

    // 容器测量子项的统一入口（唯一）：
    //   - constraints 是父级内容区的可用尺寸（已扣 padding，**未扣 margin**）；
    //   - margin 在这里统一扣除（全轴）；
    //   - 该轴无界（!is_bounded）时百分比不生效，且不能用“无界值减 margin”去构造可用尺寸；
    //   - 宽高比在百分比之后应用，最后 clamp 到 min/max。
    static glm::vec2 measure_child(Widget& child, glm::vec2 constraints)
    {
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

        if (const float ratio = child.aspect_ratio(); ratio > 0.0f) {
            if (pct_y && !pct_x) size.x = size.y * ratio;
            else size.y = size.x / ratio;
        }
        return child.clamp_size(size);
    }

    // 单控件圆角覆盖：默认用样式角色解析出的档位，个别控件可单独指定。
    void set_corner_radius(float radius) { radius_override_ = radius; }
    void clear_corner_radius() { radius_override_.reset(); }
    float corner_radius(float fallback) const { return radius_override_.value_or(fallback); }

    // 内边距
    virtual float padding_left()   const { return padding_.left; }
    virtual float padding_right()  const { return padding_.right; }
    virtual float padding_top()    const { return padding_.top; }
    virtual float padding_bottom() const { return padding_.bottom; }

    // 外边距
    virtual float margin_left()    const { return margin_.left; }
    virtual float margin_right()   const { return margin_.right; }
    virtual float margin_top()     const { return margin_.top; }
    virtual float margin_bottom()  const { return margin_.bottom; }

    // 三个布尔决定交互行为，组合语义（clips_children 见上方）：
    //   hit_testable=true   命中到此为止，path 末端是它自己（可接点击、可拖）
    //   hit_testable=false  继续向下找子项；子项都没命中时它也不接
    //                       （Vbox 用这个让点击冒泡到父级，例如窗口内容区拖动）
    //   clips_children=true 子项绘制/命中裁剪到自身 bounds（Panel / ScrollView / Table）
    //   focusable=true      参与 Tab 顺序，可被 set_focus 获得键盘事件
    virtual bool hit_testable() const { return true; }
    virtual bool focusable() const { return false; }

    // 打开模态/对话框时建议的初始焦点：默认无偏好，由模态栈按树序找首个可聚焦控件。
    virtual Widget* preferred_focus() { return nullptr; }

    // Tab 顺序：>0 为显式顺序（小的先）；0 按树序（默认）；<0 不参与 Tab（仍可点击/程序聚焦）。
    void set_tab_order(int order) { tab_order_ = order; }
    int tab_order() const { return tab_order_; }
    virtual bool on_mouse_move(glm::vec2 pos) { (void)pos; return false; }
    virtual void on_hover(glm::vec2 pos) { (void)pos; }
    virtual void on_mouse_leave() {}
    virtual void on_mouse_enter() {}
    virtual bool on_mouse_down(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_mouse_up(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    // 右键单击：不触发 on_click，也不抢焦点。
    virtual bool on_right_click(glm::vec2 pos) { (void)pos; return false; }
    // 双击：同目标、同按钮、时间与位置都在阈值内。
    virtual bool on_double_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    // 长按：按住超过阈值且指针未移出控件。
    virtual bool on_long_press(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_scroll(glm::vec2 pos, glm::vec2 delta) { (void)pos; (void)delta; return false; }
    virtual bool on_key_down(event::KeyCode key, event::Modifier mods) { (void)key; (void)mods; return false; }
    virtual bool on_key_up(event::KeyCode key, event::Modifier mods) { (void)key; (void)mods; return false; }
    virtual bool on_char(char32_t codepoint) { (void)codepoint; return false; }
    virtual void on_focus_gained() { focused_ = true; }
    virtual void on_focus_lost() { focused_ = false; }

    // 布局脏标记：任何尺寸/子项变化都会沿 parent 链向上传播，并使测量缓存失效
    // 布局进行中（LayoutPassScope）时只失效缓存、不重新标记脏：arrange 期间的几何写入是
    // “布局结果”，不是“请再排一次”的请求，否则每轮布局结束树总是脏的。
    void mark_layout_dirty()
    {
        if (!in_layout_pass()) layout_dirty_ = true;
        invalidate_measure();
        if (parent_) parent_->mark_layout_dirty();
    }
    bool layout_dirty() const { return layout_dirty_; }
    void clear_layout_dirty() { layout_dirty_ = false; }
    virtual void invalidate_measure() {}

protected:
    // 构造期的结构性默认内边距（Vbox/Hbox 的 4px）：不锁定，主题仍可接管。
    // 与 set_padding() 的区别就在于此——后者是调用方的显式意图。
    void set_default_padding(float all) { padding_ = {all, all, all, all}; }

    Rect global_;
    Rect local_;
    Align align_self_ = Align::top_left();
    bool has_align_ = false;
    Widget* parent_ = nullptr;
    // 子项存储统一放基类，派生类通过 adopt() 挂入（不要自持一份）。
    std::vector<std::unique_ptr<Widget>> children_;
    UIContext* context_ = nullptr;
    std::shared_ptr<const bool> alive_token_ = std::make_shared<const bool>(true);
    Edges padding_{};
    bool padding_override_ = false;
    Edges margin_{};
    float flex_grow_ = 0.0f;
    bool absolute_ = false;
    glm::vec2 absolute_pos_{0.0f, 0.0f};
    int z_index_ = 0;
    glm::vec2 size_percent_{-1.0f, -1.0f};
    bool fill_x_ = false;
    bool fill_y_ = false;
    float aspect_ratio_ = 0.0f;
    glm::vec2 min_size_{0.0f, 0.0f};
    glm::vec2 max_size_{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

public:
    // 每帧钩子：推进状态过渡，再交给控件自己的动画钩子。
    // 非虚，避免派生类覆写时漏调基类；自定义动画写 on_update。
    void update(float dt)
    {
        advance_transition(dt);
        on_update(dt);
    }

    // 状态样式：替代直接 ui_theme().style(...)。
    // 首次调用或 transition_ms == 0 时立即到位；状态变化时从当前插值结果出发，
    // 中途反向（hover→normal→hover）不会跳变。实现在 widget.cpp。
    BoxStyle visual_style(StyleRole role, ControlState state) const;

    bool transition_active() const;

protected:
    virtual void on_update(float /*dt*/) {}

private:
    float transition_duration() const;
    BoxStyle current_transition_style() const;
    void advance_transition(float dt);

    bool visible_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    bool debug_ = false;
    std::optional<float> radius_override_;
    bool layout_dirty_ = true;
    int tab_order_ = 0;
    // visual_style() 在 const 的 append_to 中惰性更新过渡状态。
    mutable std::unique_ptr<StyleTransition> transition_;
};
} // namespace xel::ui
