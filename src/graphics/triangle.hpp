#pragma once

#include "drawable.hpp"

#include <glm/glm.hpp>

namespace xel::graphics
{
    class Triangle : public Drawable
    {
    public:
        Triangle(float x0, float y0, float x1, float y1, float x2, float y2, glm::vec4 color)
        : p0_{x0, y0}, p1_{x1, y1}, p2_{x2, y2}, color_{color}
        {}

        void append_to(DrawBatch& batch) const override
        {
            uint16_t base = static_cast<uint16_t>(batch.vertices.size());
            batch.vertices.push_back({p0_, color_, {0,0}});
            batch.vertices.push_back({p1_, color_, {0,0}});
            batch.vertices.push_back({p2_, color_, {0,0}});

            batch.indices.push_back(base);
            batch.indices.push_back(base + 1);
            batch.indices.push_back(base + 2);
        }

    private:
        glm::vec2 p0_, p1_, p2_;
        glm::vec4 color_;
    };

} // namespace xel::graphics
