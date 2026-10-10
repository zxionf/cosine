#pragma once

#include "vertex.hpp"

#include <vector>

namespace xel::graphics
{
    struct DrawBatch
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        // TextureId texture;
        // scissor bendmode
    };

    class Drawable
    {
    public:
        virtual ~Drawable() = default;
        virtual void append_to(DrawBatch& batch) const = 0;
    };
} // namespace xel::graphics