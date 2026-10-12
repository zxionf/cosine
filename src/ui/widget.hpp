#pragma once

#include "../graphics/drawable.hpp"
#include "rect.hpp"

namespace xel::ui
{
    class Widget : public graphics::Drawable
    {
    public:
        virtual ~Widget() = default;

        virtual float x()       const = 0;
        virtual float y()       const = 0;
        virtual float width()   const = 0;
        virtual float height()  const = 0;

        Rect local_bounds() const { return local_; }

        virtual void set_pos(float x, float y) { local_.pos = {x, y}; }
        virtual void set_size(float width, float height) { local_.size = {width, height}; }
        virtual void set_pos(glm::vec2 pos) { local_.pos = pos; }
        virtual void set_size(glm::vec2 size) { local_.size = size; }

        virtual void debug() {}
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