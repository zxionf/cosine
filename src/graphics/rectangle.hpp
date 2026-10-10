#pragma once

#include "drawable.hpp"

#include <glm/glm.hpp>

namespace xel::graphics
{
    class Rectangle : public Drawable
    {
    public:
        Rectangle(float x, float y, float w, float h, glm::vec4 color)
        : x_{x}, y_{y}, w_{w}, h_{h}, color_{color}
        {}

        void append_to(DrawBatch& batch) const override
        {
            uint16_t base = static_cast<uint16_t>(batch.vertices.size());
            batch.vertices.push_back({{x_,      y_     }, color_, {0,0}});
            batch.vertices.push_back({{x_ + w_, y_     }, color_, {0,0}});
            batch.vertices.push_back({{x_ + w_, y_ + h_}, color_, {0,0}});
            batch.vertices.push_back({{x_,      y_ + h_}, color_, {0,0}});

            batch.indices.insert(batch.indices.end(), {
                base,
                static_cast<uint16_t>(base + 1),
                static_cast<uint16_t>(base + 2),
                static_cast<uint16_t>(base + 2),
                static_cast<uint16_t>(base + 3),
                base
            });
        }

    private:
        float x_, y_, w_, h_;
        glm::vec4 color_;
    };
} // namespace xel::graphics