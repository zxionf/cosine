#pragma once

#include "graphics/drawable.hpp"
#include "ui/rect.hpp"
#include "ui/align.hpp"
#include "ui/box_metrics.hpp"
#include "ui/interaction.hpp"
#include "ui/layout_metrics.hpp"
#include "ui/layout_params.hpp"
#include "ui/style_types.hpp"
#include "ui/visual_state.hpp"
#include "events/key_code.hpp"

#include <algorithm>
#include <cstdint>
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

// 布局轮次（UI-34）：UIRoot::layout() 每轮推进一次。
// 容器的 measure 缓存带这个版本键：只有“同一轮布局内”才会命中，
// 这样既能让 arrange 复用 measure 结果（少测一次），又不会跨轮读到旧值。
std::uint64_t layout_epoch();
void advance_layout_epoch();

class Widget : public graphics::Drawable
{
public:
    Widget();
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    virtual ~Widget();

    graphics::TextureSlot texture_slot() const override { return graphics::TextureSlot::Texture; }

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
    Widget* parent() const { return parent_; }
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

    // 可见性三态的定义在 layout_params.hpp（UI-48）；别名保持 `Widget::Display::Flow` 写法不变。
    using Display = ::xel::ui::Display;

    Display display() const { return !visible_ ? Display::Hidden : layout_.display; }
    void set_display(Display value)
    {
        layout_.display = value == Display::Hidden ? Display::Flow : value;
        visible_ = value != Display::Hidden;
        mark_layout_dirty();
    }

    // 溢出策略（UI-33）：
    //   Visible 内容可以超出自身（默认）
    //   Clip    绘制/命中裁剪到自身 bounds
    //   Scroll  自带滚动（ScrollView）
    // clips_children() 由它派生，不再各控件各写一份。
    enum class Overflow
    {
        Visible,
        Clip,
        Scroll,
    };
    virtual Overflow overflow() const { return Overflow::Visible; }
    // 容器是否把子控件裁剪在自身范围内（命中测试与渲染共用同一语义）。
    // Clip 与 Scroll 都裁剪，区别只是 Scroll 自带滚动条。
    // 非虚（UI-47）：它是 overflow() 的派生，全仓 0 个覆写；覆写它只会与覆写 overflow() 打架。
    bool clips_children() const { return overflow() != Overflow::Visible; }

    // 内容实际占用的尺寸（容器在 arrange 里写入）；用于判断是否溢出。
    glm::vec2 content_extent() const { return content_extent_; }
    // 内容是否超出自身尺寸（仅在容器 arrange 后有意义）。
    bool content_overflows() const
    {
        constexpr float kEps = 0.5f;
        return content_extent_.x > global_.size.x + kEps || content_extent_.y > global_.size.y + kEps;
    }

    bool visible() const { return visible_; }
    bool is_hovered() const { return input.hovered; }
    void set_hovered(bool hovered) { input.hovered = hovered; }

    // 焦点由基类持有：控件不再各自保存 focused_，样式解析与焦点环统一读这里。
    bool is_focused() const { return input.focused; }
    void set_focused(bool focused) { input.focused = focused; }
    // 焦点可见性由 FocusPolicy 决定；样式解析与焦点环实现在 widget.cpp。
    bool focus_visible() const;
    void append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const;
    void set_visible(bool visible) { visible_ = visible; mark_layout_dirty(); }
    void debug() { debug_ = true; }
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
    void set_align(Align a) { layout_.align_self = a; layout_.has_align = true; mark_layout_dirty(); }
    std::optional<Align> align_self() const
    {
        return layout_.has_align ? std::optional<Align>(layout_.align_self) : std::nullopt;
    }

    // 尺寸与对齐规格的不可变快照（UI-36）：把 align / flex / pct / aspect / fill 收敛成一份数据，
    // 便于调试面板展示与测试断言。存储仍然是「单一真相」的分散字段，set_layout_spec() 是转发。
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
    };
    LayoutSpec layout_spec() const
    {
        return {align_self(), layout_.flex_grow, layout_.size_percent, layout_.aspect_ratio,
                layout_.fill_x, layout_.fill_y, layout_.min_size, layout_.max_size};
    }
    void set_layout_spec(const LayoutSpec& spec)
    {
        if (spec.align) set_align(*spec.align);
        set_flex_grow(spec.flex_grow);
        set_size_percent(spec.size_percent);
        set_aspect_ratio(spec.aspect_ratio);
        set_fill(spec.fill_x, spec.fill_y);
        set_min_size(spec.min_size);
        set_max_size(spec.max_size);
    }

    // 内边距：显式调用即「锁住」该值，主题重刷（set_context / UIRoot::set_theme）不再覆盖。
    // 容器把主题值套进来时走 apply_theme_padding()，那条路径会跳过已锁定的控件。
    void    set_padding(Edges edges)    { box.padding = edges; box.padding_override = true; mark_layout_dirty(); }
    void    set_padding(float all)      { set_padding(Edges{all, all, all, all}); }
    Edges   padding() const             { return box.padding; }
    bool    padding_override() const    { return box.padding_override; }
    // 交还给主题：之后 apply_theme_padding() 才会重新生效。
    void    clear_padding_override()    { box.padding_override = false; }
    // 主题套用入口（Panel / TabView 的 set_context 用）：显式设定过的内边距不覆盖。
    void    apply_theme_padding(float all)
    {
        if (box.padding_override) return;
        box.padding = {all, all, all, all};
        mark_layout_dirty();
    }
    void    set_margin(Edges edges)     { box.margin = edges; mark_layout_dirty(); }
    void    set_margin(float all)       { box.margin = {all, all, all, all}; mark_layout_dirty(); }
    Edges   margin() const              { return box.margin; }

    void    set_flex_grow(float grow)   { layout_.flex_grow = grow < 0.0f ? 0.0f : grow; mark_layout_dirty(); }
    float   flex_grow() const           { return layout_.flex_grow; }

    // 绝对定位：脱离流式布局，按 (x, y) 相对父内容区定位；容器不计入其尺寸。
    void set_absolute(bool value)
    {
        if (value && layout_.display != Display::Positioned) layout_.absolute_pos = local_.pos; // 兼容：从当前位置开始
        layout_.display = value ? Display::Positioned : Display::Flow;
        mark_layout_dirty();
    }
    bool absolute() const { return layout_.display == Display::Positioned; }

    // 相对偏移与全局位置分离：arrange 会改写 local_.pos（全局），
    // 若 absolute 子项读 x()/y() 会每帧累加父偏移。
    void set_absolute_pos(glm::vec2 pos)
    {
        layout_.absolute_pos = pos;
        layout_.display = Display::Positioned;
        mark_layout_dirty();
    }
    glm::vec2 absolute_pos() const { return layout_.absolute_pos; }

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
    void set_min_size(glm::vec2 size) { layout_.min_size = size; mark_layout_dirty(); }
    void set_max_size(glm::vec2 size) { layout_.max_size = size; mark_layout_dirty(); }
    glm::vec2 min_size() const { return layout_.min_size; }
    glm::vec2 max_size() const { return layout_.max_size; }
    glm::vec2 clamp_size(glm::vec2 size) const { return glm::clamp(size, layout_.min_size, layout_.max_size); }

    // 叶子的自然尺寸（UI-32）：构造期确定的 preferred size，与 arrange 的几何分离。
    // measure() 用它而不是 local_.size —— 后者是最近一次 arrange 的结果，被 stretch
    // 之后会「记住」放大值，再也缩不回去（P1-6）。
    void set_preferred_size(glm::vec2 size) { layout_.preferred = size; mark_layout_dirty(); }
    glm::vec2 preferred_size() const { return layout_.preferred; }

    // 外部显式指定尺寸：几何与自然尺寸一起改。
    // 调试面板这类「用户改 Size」用它；布局内部的 arrange 不要调（arrange 只写几何）。
    void request_size(glm::vec2 size)
    {
        layout_.preferred = size;
        set_size(size);
    }

    // 尺寸规格：每轴占父级可用空间的比例（>0 启用）与宽高比（>0 启用）。
    void set_size_percent(glm::vec2 pct) { layout_.size_percent = pct; mark_layout_dirty(); }
    void set_size_percent(float x, float y) { set_size_percent({x, y}); }
    glm::vec2 size_percent() const { return layout_.size_percent; }

    // Fill 尺寸模式：沿该轴吃满父级内容区。
    // 与 size_percent=1 的几何结果相同，但语义显式：
    //   - measure_child 在扣完 margin 后直接用可用尺寸（不再依赖“返回 constraints”的骗法）；
    //   - 父级据此知道“这个子项不该把我的自然尺寸撞大”（预算在 UI-32 统一）。
    void set_fill(bool x, bool y)
    {
        layout_.fill_x = x;
        layout_.fill_y = y;
        mark_layout_dirty();
    }
    bool fill_x() const { return layout_.fill_x; }
    bool fill_y() const { return layout_.fill_y; }
    void set_aspect_ratio(float ratio) { layout_.aspect_ratio = ratio > 0.0f ? ratio : 0.0f; mark_layout_dirty(); }
    float aspect_ratio() const { return layout_.aspect_ratio; }

    // 容器测量子项的统一入口（唯一）：
    //   - constraints 是父级内容区的可用尺寸（已扣 padding，**未扣 margin**）；
    //   - margin 在这里统一扣除（全轴）；
    //   - 该轴无界（!is_bounded）时百分比不生效，且不能用“无界值减 margin”去构造可用尺寸；
    //   - 宽高比在百分比之后应用，最后 clamp 到 min/max；
    //   - 结果带 (constraints, 布局轮次) 缓存（UI-34）：同一轮内重复调用（measure → arrange）不重测。
    static glm::vec2 measure_child(Widget& child, glm::vec2 constraints)
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

    // 单控件圆角覆盖：默认用样式角色解析出的档位，个别控件可单独指定。
    void set_corner_radius(float radius) { radius_override_ = radius; }
    void clear_corner_radius() { radius_override_.reset(); }
    float corner_radius(float fallback) const { return radius_override_.value_or(fallback); }

    // 内边距
    // 非虚（UI-47）：全仓 0 个覆写；派生类要改度量应当走 box/padding 部件，不再开新的虚函数。
    float padding_left()   const { return box.padding.left; }
    float padding_right()  const { return box.padding.right; }
    float padding_top()    const { return box.padding.top; }
    float padding_bottom() const { return box.padding.bottom; }

    // 外边距
    float margin_left()    const { return box.margin.left; }
    float margin_right()   const { return box.margin.right; }
    float margin_top()     const { return box.margin.top; }
    float margin_bottom()  const { return box.margin.bottom; }

    // 文字基线相对控件顶边的偏移（UI-36）：默认无文本 → 用高度一半（居中）。
    // Label/Button 这类有文字的控件覆写 baseline_offset_for()，供 Align::baseline_align() 使用。
    virtual float baseline_offset_for(float h) const { return h * 0.5f; }
    float baseline_offset() const { return baseline_offset_for(height()); }

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
    virtual void on_focus_gained() { input.focused = true; }
    virtual void on_focus_lost() { input.focused = false; }

    // 布局脏标记：任何尺寸/子项变化都会沿 parent 链向上传播，并使测量缓存失效。
    // 布局进行中（LayoutPassScope）时只失效缓存、不重新标记脏：arrange 期间的几何写入是
    // “布局结果”，不是“请再排一次”的请求。
    //
    // 早退（UI-34）：不变量是“子树已通知过祖先链”——通知过就不再重复向上走。
    //   - 通知发生：mark_layout_dirty 向父级传递时置位 `notified_`；
    //   - 通知重置：本控件（或它的宿主）本轮被测量/被统计时清位（mark_measured）；
    //   - 整轮 UIRoot::layout() 收尾统一清位（clear_layout_dirty_recursive）。
    // 不能拿 layout_dirty_ 做判据：它表示“本轮请求重排”，与失效传播不是一回事。
    void mark_layout_dirty()
    {
        if (!in_layout_pass()) layout_dirty_ = true;
        invalidate_measure();
        if (notified_) return;
        notified_ = true;
        if (parent_) parent_->mark_layout_dirty();
    }
    bool layout_dirty() const { return layout_dirty_; }
    // 清除脏标记与「已通知祖先」标记。
    // 必须递归（UI-45）：早退依赖 notified_，只清自己的时候子项的 notified_ 仍为 true，
    // 后续子项变更会被早退吃掉——表现为 set_pos() 后 root 不再是 dirty。
    void clear_layout_dirty() { clear_layout_dirty_recursive(); }
    // 递归清除脏标记：UIRoot::layout() 收尾时用。
    void clear_layout_dirty_recursive()
    {
        layout_dirty_ = false;
        notified_ = false;
        for (const auto& child : children_) child->clear_layout_dirty_recursive();
        for (const auto& overlay : overlays()) overlay->clear_layout_dirty_recursive();
    }
    // 测量缓存失效（单节点）。派生类只管自己的派生度量。
    virtual void invalidate_measure() { measure_valid_ = false; }
    // 递归失效：主题/密度/字号切换后必须走这个（否则容器拿旧缓存 + 新主题 = 尺寸错位）。
    void invalidate_measure_recursive()
    {
        invalidate_measure();
        for (const auto& child : children_) child->invalidate_measure_recursive();
        for (const auto& overlay : overlays()) overlay->invalidate_measure_recursive();
    }

    // 测量缓存的三个入口（派生类 measure() 用）：
    //   - 命中条件：仍有效、同一轮布局、约束相同；
    //   - 布局轮次是唯一跨轮失效手段，不依赖子项是否在 arrange 里标脏。
    bool measure_cache_hit(glm::vec2 constraints) const
    {
        return measure_valid_ && measure_epoch_ == layout_epoch()
            && constraints == measure_constraints_;
    }
    glm::vec2 measure_cached() const { return measure_cache_; }
    void store_measure(glm::vec2 constraints, glm::vec2 result)
    {
        measure_cache_ = result;
        measure_constraints_ = constraints;
        measure_epoch_ = layout_epoch();
        measure_valid_ = true;
    }
    // 测量已失效（调试面板与断言用）。
    bool measure_valid() const { return measure_valid_; }
    // 本轮已统计过（测过或按规则跳过）：清除“已通知祖先”标记，下次变更重新传播。
    // 容器对该轮不参与流式的子项（Hidden / Positioned）也要调用它。
    void mark_measured()
    {
        measure_valid_ = true;
        notified_ = false;
    }

protected:
    // 构造期的结构性默认内边距（Vbox/Hbox 的 4px）：不锁定，主题仍可接管。
    // 与 set_padding() 的区别就在于此——后者是调用方的显式意图。
    void set_default_padding(float all) { box.padding = {all, all, all, all}; }
    // 容器 arrange 时记录内容实际占用尺寸，供 content_overflows() 查询。
    void set_content_extent(glm::vec2 extent) { content_extent_ = extent; }

    // ── 四个部件（UI-48）─────────────────────────────────────────────
    // 以前这里是 22 个平铺字段（布局意图 / 盒模型 / 交互状态 / 样式过渡），
    // 现在各自归组：字段语义不变、访问器签名不变，43 个控件无需修改。
    LayoutParams layout_;   // preferred/min/max/pct/fill/aspect/flex/align/absolute/display
    BoxMetrics   box;      // padding（含 override）/ margin
    Interaction  input;    // enabled/hovered/pressed/dragging/selected/focused
    VisualState  visual;   // 样式过渡（惰性、R4 将去堆）

    Rect global_;
    Rect local_;
    Widget* parent_ = nullptr;
    // 子项存储统一放基类，派生类通过 adopt() 挂入（不要自持一份）。
    std::vector<std::unique_ptr<Widget>> children_;
    UIContext* context_ = nullptr;
    std::shared_ptr<const bool> alive_token_ = std::make_shared<const bool>(true);
    int z_index_ = 0;

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
    // 中途反向（hover→normal→hover）不会跳变。
    // 一个控件一帧内可以请求多个角色（如 Slider 的 Track + Accent + Tooltip）：
    // 每个角色有独立过渡槽，互不影响。实现在 widget.cpp。
    BoxStyle visual_style(StyleRole role, ControlState state) const;

    bool transition_active() const;

protected:
    virtual void on_update(float /*dt*/) {}

private:
    float transition_duration() const;
    BoxStyle current_transition_style(StyleRole role) const;
    void advance_transition(float dt);

    bool visible_ = true;
    // 交互状态（hovered/focused）已移入 input 部件（UI-48）；这里只留下不与部件重叠的布尔。
    bool debug_ = false;
    std::optional<float> radius_override_;
    bool layout_dirty_ = true;
    int tab_order_ = 0;
    glm::vec2 content_extent_{0.0f, 0.0f};
    // measure 缓存（UI-34）：约束 + 布局轮次双重键。
    mutable glm::vec2 measure_cache_{0.0f, 0.0f};
    mutable glm::vec2 measure_constraints_{0.0f, 0.0f};
    mutable std::uint64_t measure_epoch_ = 0;
    mutable bool measure_valid_ = false;
    // 本轮是否已把“失效”告知祖先链（UI-34 早退）。
    bool notified_ = false;
    // visual_style() 在 const 的 append_to 中惰性更新过渡状态（部件内 mutable）。
};
} // namespace xel::ui
