#pragma once

#include "../graphics/drawable.hpp"
#include "../graphics/line.hpp"
#include "rect.hpp"
#include "align.hpp"

#include <memory>
#include <optional>

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

    virtual void set_pos(float x, float y) { local_.pos = {x, y}; }
    virtual void set_size(float width, float height) { local_.size = {width, height}; }
    virtual void set_pos(glm::vec2 pos) { local_.pos = pos; }
    virtual void set_size(glm::vec2 size) { local_.size = size; }
    virtual void set_global_bounds(Rect bounds) { global_ = bounds; }

    virtual void add(std::unique_ptr<Widget> child) {}
    virtual const std::vector<std::unique_ptr<Widget>>& children() const { static const std::vector<std::unique_ptr<Widget>> empty; return empty; }
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
    void set_align(Align a) { align_self_ = a; has_align_ = true; }
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
    virtual void on_mouse_move(glm::vec2 pos) {}
    virtual void on_mouse_leave() {}
    virtual void on_mouse_enter() {}

protected:
    Rect global_;
    Rect local_;
    Align align_self_ = Align::top_left();
    bool has_align_ = false;
private:
    bool visible_ = true;
    bool debug_ = false;
};
} // namespace xel::ui