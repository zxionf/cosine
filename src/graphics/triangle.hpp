#pragma once

#include "drawable.hpp"
#include "font_atlas.hpp"

#include <glm/glm.hpp>

namespace xel::graphics
{
class Triangle : public Drawable
{
public:
    Triangle(float x0, float y0, float x1, float y1, float x2, float y2, glm::vec4 color)
    : p0_{x0, y0}, p1_{x1, y1}, p2_{x2, y2}, color_{color}
    {}

    void append_to(DrawList& list) const override
    {
        list.set_texture(TextureSlot::Texture);
        auto& batch = list.batch();
        const glm::vec2 uv = active_font_atlas() ? active_font_atlas()->white_uv() : glm::vec2{0.0f};
        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.add_vertex(p0_, color_, uv);
        batch.add_vertex(p1_, color_, uv);
        batch.add_vertex(p2_, color_, uv);

        batch.indices.push_back(base);
        batch.indices.push_back(base + 1);
        batch.indices.push_back(base + 2);
    }

    TextureSlot texture_slot() const override { return TextureSlot::Texture; }

private:
    glm::vec2 p0_, p1_, p2_;
    glm::vec4 color_;
};

} // namespace xel::graphics
