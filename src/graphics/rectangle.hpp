#pragma once

#include "drawable.hpp"
#include "font_atlas.hpp"

#include <functional>

#include <glm/glm.hpp>

namespace xel::graphics
{
class Rectangle : public Drawable
{
public:
    Rectangle(float x, float y, float w, float h, glm::vec4 color)
    : x_{x}, y_{y}, w_{w}, h_{h}, color_{color}
    {}

    void append_to(DrawList& list) const override
    {
        list.set_texture(TextureSlot::Texture);
        auto& batch = list.batch();
        const glm::vec2 uv = active_font_atlas() ? active_font_atlas()->white_uv() : glm::vec2{0.0f};
        // 管线使用预乘 alpha 混合（One / OneMinusSrcAlpha）；
        // 半透明矩形必须先把 rgb 乘以 alpha，否则会显得过亮。
        const glm::vec4 color{color_.r * color_.a, color_.g * color_.a,
                              color_.b * color_.a, color_.a};
        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.vertices.push_back({{x_,      y_     }, color, uv});
        batch.vertices.push_back({{x_ + w_, y_     }, color, uv});
        batch.vertices.push_back({{x_ + w_, y_ + h_}, color, uv});
        batch.vertices.push_back({{x_,      y_ + h_}, color, uv});

        batch.indices.insert(batch.indices.end(), {
            base,
            base + 1,
            base + 2,
            base + 2,
            base + 3,
            base
        });
    }

    bool hit_test(float x, float y) const
    {
        return x >= x_ && x <= x_ + w_ && y >= y_ && y <= y_ + h_;
    }

    TextureSlot texture_slot() const override { return TextureSlot::Texture; }

    // bool on_mouse

    std::function<void()> on_click_;
private:
    float x_, y_, w_, h_;
    glm::vec4 color_;
};
} // namespace xel::graphics
