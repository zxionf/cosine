#pragma once

#include "vertex.hpp"

#include <vector>

namespace xel::graphics
{
enum class TextureSlot : uint32_t
{
    White = 0,
    Texture = 1,
};

struct DrawBatch
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    TextureSlot texture_slot = TextureSlot::White;
    glm::vec4 scissor{0.0f, 0.0f, 0.0f, 0.0f};
};

class Drawable
{
public:
    virtual ~Drawable() = default;
    virtual void append_to(DrawBatch& batch) const = 0;
    virtual TextureSlot texture_slot() const { return TextureSlot::White; }
};
} // namespace xel::graphics