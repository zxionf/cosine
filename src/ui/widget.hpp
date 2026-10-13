#pragma once

#include "../graphics/drawable.hpp"
#include "../graphics/line.hpp"
#include "rect.hpp"

#include <memory>

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

        Rect local_bounds() const { return local_; }

        virtual void set_pos(float x, float y) { local_.pos = {x, y}; }
        virtual void set_size(float width, float height) { local_.size = {width, height}; }
        virtual void set_pos(glm::vec2 pos) { local_.pos = pos; }
        virtual void set_size(glm::vec2 size) { local_.size = size; }

        virtual void add(std::unique_ptr<Widget> child) {}
        virtual void debug() {}
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
    private:
        Rect local_;
        bool visible_ = true;
    };
} // namespace xel::ui