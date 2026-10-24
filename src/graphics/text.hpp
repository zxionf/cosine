#pragma once

#include "drawable.hpp"
#include "font_atlas.hpp"

#include <glm/glm.hpp>
#include <string>

namespace xel::graphics
{
class Text : public Drawable
{
public:
    Text(float x, float y, float size, std::string content, glm::vec4 color)
    : x_{x}, y_{y}, size_{size}, content_{std::move(content)}, color_{color}
    {}

    void append_to(DrawList& list) const override
    {
        if (auto* atlas = active_font_atlas()) {
            atlas->append_text(list, x_, y_, size_, content_, color_);
        }
    }

    TextureSlot texture_slot() const override { return TextureSlot::Texture; }

    glm::vec2 measure() const
    {
        if (auto* atlas = active_font_atlas()) {
            return atlas->measure(content_, size_);
        }
        return {static_cast<float>(content_.size()) * size_ * 0.5f, size_};
    }

private:
    float x_, y_, size_;
    std::string content_;
    glm::vec4 color_;
};
} // namespace xel::graphics
