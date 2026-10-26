#pragma once

#include "../graphics/drawable.hpp"
#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "rect.hpp"
#include "align.hpp"
#include "uicontext.hpp"
#include "style.hpp"
#include "../events/key_code.hpp"

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
    void set_min_size(glm::vec2 size) { min_size_ = size; mark_layout_dirty(); }
    void set_max_size(glm::vec2 size) { max_size_ = size; mark_layout_dirty(); }
    glm::vec2 min_size() const { return min_size_; }
    glm::vec2 max_size() const { return max_size_; }
    glm::vec2 clamp_size(glm::vec2 size) const { return glm::clamp(size, min_size_, max_size_); }

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
    virtual bool on_mouse_move(glm::vec2 pos) { (void)pos; return false; }
    virtual void on_hover(glm::vec2 pos) { (void)pos; }
    virtual void on_mouse_leave() {}
    virtual void on_mouse_enter() {}
    virtual bool on_mouse_down(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_mouse_up(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
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
    glm::vec2 min_size_{0.0f, 0.0f};
    glm::vec2 max_size_{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

private:
    bool visible_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    bool debug_ = false;
    std::optional<float> radius_override_;
    bool layout_dirty_ = true;
};
} // namespace xel::ui
