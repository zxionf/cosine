#pragma once

#include "graphics/drawable.hpp"
#include "ui/rect.hpp"
#include "ui/align.hpp"
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

    void set_padding(Edges edges) { padding_ = edges; mark_layout_dirty(); }
    void set_padding(float all) { padding_ = {all, all, all, all}; mark_layout_dirty(); }
    Edges padding() const { return padding_; }
    void set_margin(Edges edges) { margin_ = edges; mark_layout_dirty(); }
    void set_margin(float all) { margin_ = {all, all, all, all}; mark_layout_dirty(); }
    Edges margin() const { return margin_; }

    void set_flex_grow(float grow) { flex_grow_ = grow < 0.0f ? 0.0f : grow; mark_layout_dirty(); }
    float flex_grow() const { return flex_grow_; }

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
    void set_min_size(glm::vec2 size) { min_size_ = size; mark_layout_dirty(); }
    void set_max_size(glm::vec2 size) { max_size_ = size; mark_layout_dirty(); }
    glm::vec2 min_size() const { return min_size_; }
    glm::vec2 max_size() const { return max_size_; }
    glm::vec2 clamp_size(glm::vec2 size) const { return glm::clamp(size, min_size_, max_size_); }

    // 尺寸规格：每轴占父级可用空间的比例（>0 启用）与宽高比（>0 启用）。
    void set_size_percent(glm::vec2 pct) { size_percent_ = pct; mark_layout_dirty(); }
    void set_size_percent(float x, float y) { set_size_percent({x, y}); }
    glm::vec2 size_percent() const { return size_percent_; }
    void set_aspect_ratio(float ratio) { aspect_ratio_ = ratio > 0.0f ? ratio : 0.0f; mark_layout_dirty(); }
    float aspect_ratio() const { return aspect_ratio_; }

    // 容器测量子项的统一入口：先应用百分比与宽高比，再 clamp 到 min/max。
    // constraints 是父级提供的可用空间；该轴无界（>= 1e8）时百分比不生效。
    // 宽高比优先用宽推高；只指定了百分比高时用高推宽。
    static glm::vec2 measure_child(Widget& child, glm::vec2 constraints)
    {
        constexpr float kUnbounded = 1.0e8f;
        const glm::vec2 pct = child.size_percent();
        const bool pct_x = pct.x > 0.0f && constraints.x < kUnbounded;
        const bool pct_y = pct.y > 0.0f && constraints.y < kUnbounded;

        glm::vec2 spec = constraints;
        if (pct_x) spec.x = constraints.x * pct.x;
        if (pct_y) spec.y = constraints.y * pct.y;

        glm::vec2 size = child.measure(spec);
        if (pct_x) size.x = spec.x;
        if (pct_y) size.y = spec.y;

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
    void mark_layout_dirty()
    {
        layout_dirty_ = true;
        invalidate_measure();
        if (parent_) parent_->mark_layout_dirty();
    }
    bool layout_dirty() const { return layout_dirty_; }
    void clear_layout_dirty() { layout_dirty_ = false; }
    virtual void invalidate_measure() {}

protected:
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
    Edges margin_{};
    float flex_grow_ = 0.0f;
    bool absolute_ = false;
    glm::vec2 absolute_pos_{0.0f, 0.0f};
    int z_index_ = 0;
    glm::vec2 size_percent_{-1.0f, -1.0f};
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
