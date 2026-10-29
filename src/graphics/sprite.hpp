#pragma once

#include "drawable.hpp"

#include <glm/glm.hpp>

#include <cstdint>

namespace xel::graphics
{
// 带 UV 与纹理槽的矩形图元（CO-2）。
// 纯色图元用 Rectangle（采样字体图集里的 white_uv）；图片用这里，槽号由调用方给。
class Sprite : public Drawable
{
public:
    Sprite(float x, float y, float width, float height, uint32_t texture_slot,
           glm::vec4 color = glm::vec4{1.0f},
           glm::vec4 uv = glm::vec4{0.0f, 0.0f, 1.0f, 1.0f})
        : x_{x}, y_{y}, w_{width}, h_{height}, slot_{texture_slot}, color_{color}, uv_{uv}
    {}

    void append_to(DrawList& list) const override
    {
        list.set_texture_slot(slot_);
        auto& batch = list.batch();

        // 管线是预乘 alpha（One / OneMinusSrcAlpha），与 Rectangle 保持一致。
        const glm::vec4 color{color_.r * color_.a, color_.g * color_.a,
                              color_.b * color_.a, color_.a};

        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.add_vertex({x_,      y_     }, color, {uv_.x, uv_.y});
        batch.add_vertex({x_ + w_, y_     }, color, {uv_.z, uv_.y});
        batch.add_vertex({x_ + w_, y_ + h_}, color, {uv_.z, uv_.w});
        batch.add_vertex({x_,      y_ + h_}, color, {uv_.x, uv_.w});

        batch.indices.insert(batch.indices.end(), {
            base,
            base + 1,
            base + 2,
            base + 2,
            base + 3,
            base
        });
    }

    // 图片槽号（不是 Drawable::texture_slot() 的 TextureSlot 语义）。
    uint32_t slot() const { return slot_; }

private:
    float x_ = 0.0f;
    float y_ = 0.0f;
    float w_ = 0.0f;
    float h_ = 0.0f;
    uint32_t slot_ = 0;
    glm::vec4 color_{1.0f};
    glm::vec4 uv_{0.0f, 0.0f, 1.0f, 1.0f};
};
} // namespace xel::graphics
