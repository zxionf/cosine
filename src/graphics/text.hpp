#pragma once

#include "drawable.hpp"

#include <glm/glm.hpp>

namespace xel::graphics
{
    class Text : public Drawable
    {
    public:
        Text(float x, float y, float size, std::string content, glm::vec4 color)
        : x_{x}, y_{y}, size_{size}, content_{std::move(content)}, color_{color}
        {}

        void append_to(DrawBatch& batch) const override
        {
        }


    private:
        float x_, y_, size_;
        std::string content_;
        glm::vec4 color_;
    };
} // namespace xel::graphics