#pragma once

#include "../graphics/drawable.hpp"
#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "rect.hpp"
#include "align.hpp"
#include "uicontext.hpp"
#include "style.hpp"
#include "../events/key_code.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace xel::ui
{
struct Edges
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

// 控件状态过渡缓存：只在 (role, state) 变化时重建，几何瞬时、颜色平滑。
struct StyleTransition
{
    StyleRole role = StyleRole::Control;
    ControlState state = ControlState::Normal;
    BoxStyle from{};
    BoxStyle to{};
    float progress = 1.0f; // 1 = 已完成
    bool started = false;
    std::uint64_t revision = 0; // 建立时的主题版本
};

class Widget : public graphics::Drawable
{
public:
    virtual ~Widget() = default;

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
    virtual const std::vector<std::unique_ptr<Widget>>& children() const { static const std::vector<std::unique_ptr<Widget>> empty; return empty; }
    // overlay 是顶层绘制/命中层：root 先绘制普通子项，再绘制 overlay；命中测试反向进行。
    virtual const std::vector<std::unique_ptr<Widget>>& overlays() const { static const std::vector<std::unique_ptr<Widget>> empty; return empty; }
    virtual Widget* parent() const { return parent_; }
    void set_parent(Widget* parent) { parent_ = parent; }

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
    const Theme& ui_theme() const
    {
        if (auto* ctx = context()) return ctx->theme;
        return theme();
    }
    graphics::FontAtlas* font_atlas() const
    {
        if (auto* ctx = context()) return ctx->font_atlas;
        return graphics::active_font_atlas();
    }
    // 容器是否把子控件裁剪在自身范围内（命中测试与渲染共用同一语义）
    virtual bool clips_children() const { return false; }

    bool visible() const { return visible_; }
    bool is_hovered() const { return hovered_; }
    void set_hovered(bool hovered) { hovered_ = hovered; }

    // 焦点由基类持有：控件不再各自保存 focused_，样式解析与焦点环统一读这里。
    bool is_focused() const { return focused_; }
    void set_focused(bool focused) { focused_ = focused; }
    // 焦点可见性只由 FocusPolicy 决定，与几何风格（圆角环 / 贴边框）无关：
    // Keyboard = 仅键盘操作期间显示，Always = 聚焦即显示，Never = 从不显示。
    bool focus_visible() const
    {
        if (!focused_) return false;
        const UIContext* ctx = context();
        if (ctx == nullptr) return true;
        switch (ctx->theme.focus_policy) {
            case FocusPolicy::Always: return true;
            case FocusPolicy::Never:  return false;
            case FocusPolicy::Keyboard: break;
        }
        return ctx->input_mode == InputMode::Keyboard;
    }
    // 焦点环画在控件外扩 focus_offset 的圆角环上，不压住内容边缘。
    // 经典样式（FocusStyle::Border）则回到贴控件内边缘的四条实心框：无圆角、不外扩。
    void append_focus_ring(graphics::DrawList& list, glm::vec4 rect, float radius) const
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
    void set_visible(bool visible) { visible_ = visible; mark_layout_dirty(); }
    virtual void debug() { debug_ = true; }
    bool is_debug() const { return debug_; }
    void debug_recursive()
    {
        debug();
        for (const auto& child : children()) child->debug_recursive();
    }
    void append_debug_tree(graphics::DrawList& list) const
    {
        if (is_debug()) debug_rect(list);
        for (const auto& child : children()) child->append_debug_tree(list);
        for (const auto& overlay : overlays()) overlay->append_debug_tree(list);
    }
    void debug_rect(graphics::DrawList& list) const {
        auto [x, y, w, h] = global_;
        const glm::vec4 color = hovered_ ? glm::vec4{1, 0, 1, 1} : glm::vec4{0, 1, 0, 1};
        graphics::Line top_l    {x,     y,      x + w,  y,      1.0f, color};
        graphics::Line right_l  {x + w, y,      x + w,  y + h,  1.0f, color};
        graphics::Line bottom_l {x,     y + h,  x + w,  y + h,  1.0f, color};
        graphics::Line left_l   {x,     y,      x,      y + h,  1.0f, color};
        top_l.append_to(list);
        right_l.append_to(list);
        bottom_l.append_to(list);
        left_l.append_to(list);
    }
    virtual void layout() {}
    virtual glm::vec2 measure(glm::vec2 constraints) = 0;
    virtual void arrange(Rect slot) = 0;
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
        absolute_ = value;
        mark_layout_dirty();
    }
    bool absolute() const { return absolute_; }

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
    UIContext* context_ = nullptr;
    std::shared_ptr<const bool> alive_token_ = std::make_shared<const bool>(true);
    Edges padding_{};
    Edges margin_{};
    float flex_grow_ = 0.0f;
    bool absolute_ = false;
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
    // 因此中途反向（hover→normal→hover）不会跳变。
    BoxStyle visual_style(StyleRole role, ControlState state) const
    {
        const auto& theme_value = ui_theme();
        const BoxStyle target = theme_value.style(role, state);
        const std::uint64_t revision = theme_revision();
        // 主题变化（切换主题/圆角/密度/过渡时长）：旧缓存的 from/to 已过时，
        // 直接重建成目标样式，不做过渡——否则会等到下一次 hover 才刷新。
        if (!transition_.started || transition_.revision != revision) {
            transition_ = StyleTransition{role, state, target, target, 1.0f, true, revision};
            return target;
        }
        if (transition_.role != role || transition_.state != state) {
            transition_.from = current_transition_style();
            transition_.to = target;
            transition_.role = role;
            transition_.state = state;
            transition_.progress = transition_duration() <= 0.0f ? 1.0f : 0.0f;
        }
        return current_transition_style();
    }

    bool transition_active() const { return transition_.progress < 1.0f; }

protected:
    virtual void on_update(float /*dt*/) {}

private:
    float transition_duration() const
    {
        const float ms = ui_theme().transition_ms;
        return ms > 0.0f ? ms / 1000.0f : 0.0f;
    }

    BoxStyle current_transition_style() const
    {
        return lerp_style(transition_.from, transition_.to, ease_out_quad(transition_.progress));
    }

    void advance_transition(float dt)
    {
        if (transition_.progress >= 1.0f) return;
        const float duration = transition_duration();
        if (duration <= 0.0f) {
            transition_.progress = 1.0f;
            return;
        }
        transition_.progress = std::min(1.0f, transition_.progress + dt / duration);
    }

    bool visible_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    bool debug_ = false;
    std::optional<float> radius_override_;
    bool layout_dirty_ = true;
    int tab_order_ = 0;
    // visual_style() 在 const 的 append_to 中惰性更新过渡状态。
    mutable StyleTransition transition_;
};
} // namespace xel::ui
