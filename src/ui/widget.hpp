#pragma once

#include "../graphics/drawable.hpp"
#include "../graphics/line.hpp"
#include "rect.hpp"
#include "align.hpp"

#include <memory>
#include <optional>
#include <stdexcept>

namespace xel::ui
{
class Widget : public graphics::Drawable
{
public:
    virtual ~Widget() = default;

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
    virtual Widget* parent() const { return parent_; }
    void set_parent(Widget* parent) { parent_ = parent; }

    // 容器是否把子控件裁剪在自身范围内（命中测试与渲染共用同一语义）
    virtual bool clips_children() const { return false; }

    bool visible() { return visible_; }
    virtual void debug() { debug_ = true; }
    bool is_debug() const { return debug_; }
    void debug_rect(graphics::DrawBatch& batch) const {
        auto [x, y, w, h] = local_;
        graphics::Line top_l    {x,     y,      x + w,  y,      1.0f, {0, 1, 0, 1}};
        graphics::Line right_l  {x + w, y,      x + w,  y + h,  1.0f, {0, 1, 0, 1}};
        graphics::Line bottom_l {x,     y + h,  x + w,  y + h,  1.0f, {0, 1, 0, 1}};
        graphics::Line left_l   {x,     y,      x,      y + h,  1.0f, {0, 1, 0, 1}};
        top_l.append_to(batch);
        right_l.append_to(batch);
        bottom_l.append_to(batch);
        left_l.append_to(batch);
    }

    virtual void layout() {}
    virtual glm::vec2 measure(glm::vec2 constraints) = 0;
    virtual void arrange(Rect slot) = 0;
    void set_align(Align a) { align_self_ = a; has_align_ = true; mark_layout_dirty(); }
    std::optional<Align> align_self() const { return has_align_ ? std::optional<Align>(align_self_) : std::nullopt; }

    // 内边距
    virtual float padding_left()   const { return 0; }
    virtual float padding_right()  const { return 0; }
    virtual float padding_top()    const { return 0; }
    virtual float padding_bottom() const { return 0; }

    // 外边距
    virtual float margin_left()    const { return 0; }
    virtual float margin_right()   const { return 0; }
    virtual float margin_top()     const { return 0; }
    virtual float margin_bottom()  const { return 0; }

    virtual bool hit_testable() const { return true; }
    virtual bool focusable() const { return false; }
    virtual bool on_mouse_move(glm::vec2 pos) { (void)pos; return false; }
    virtual void on_mouse_leave() {}
    virtual void on_mouse_enter() {}
    virtual bool on_mouse_down(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_mouse_up(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_click(glm::vec2 pos, int button) { (void)pos; (void)button; return false; }
    virtual bool on_scroll(glm::vec2 pos, float delta) { (void)pos; (void)delta; return false; }
    virtual bool on_key_down(int key, int mods) { (void)key; (void)mods; return false; }
    virtual bool on_key_up(int key, int mods) { (void)key; (void)mods; return false; }
    virtual bool on_char(char32_t codepoint) { (void)codepoint; return false; }
    virtual void on_focus_gained() {}
    virtual void on_focus_lost() {}

    // 布局脏标记：任何尺寸/子项变化都会沿 parent 链向上传播
    void mark_layout_dirty()
    {
        layout_dirty_ = true;
        if (parent_) parent_->mark_layout_dirty();
    }
    bool layout_dirty() const { return layout_dirty_; }
    void clear_layout_dirty() { layout_dirty_ = false; }

protected:
    Rect global_;
    Rect local_;
    Align align_self_ = Align::top_left();
    bool has_align_ = false;
    Widget* parent_ = nullptr;

private:
    bool visible_ = true;
    bool debug_ = false;
    bool layout_dirty_ = true;
};
} // namespace xel::ui
