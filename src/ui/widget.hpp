#pragma once

#include "../graphics/drawable.hpp"

namespace xel::ui
{
    class Widget : public graphics::Drawable
    {
    public:
        virtual ~Widget() = default;

        virtual float x() const = 0;
        virtual float y() const = 0;
        virtual float width() const = 0;
        virtual float height() const = 0;

        virtual void set_position(float x, float y) = 0;

        // 内边距
        virtual float padding_left()   const { return 0; }
        virtual float padding_right()  const { return 0; }
        virtual float padding_top()    const { return 0; }
        virtual float padding_bottom() const { return 0; }

        // 外边距
        virtual float margin_left()   const { return 0; }
        virtual float margin_right()  const { return 0; }
        virtual float margin_top()    const { return 0; }
        virtual float margin_bottom() const { return 0; }
    };
} // namespace xel::ui