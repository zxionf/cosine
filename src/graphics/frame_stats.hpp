#pragma once

#include "drawable.hpp"

#include <cstdint>

namespace xel::graphics
{
struct DrawStats
{
    uint32_t batches = 0;
    uint32_t draw_calls = 0;
    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t clipped_batches = 0;
};

struct FrameStats
{
    double frame_time_ms = 0.0;
    double fps = 0.0;
    DrawStats draw;
};

inline DrawStats collect_draw_stats(const DrawList& list, float viewport_width, float viewport_height)
{
    DrawStats stats;
    stats.batches = static_cast<uint32_t>(list.batches().size());

    for (const auto& batch : list.batches()) {
        if (batch.indices.empty()) continue;

        ++stats.draw_calls;
        stats.vertices += static_cast<uint32_t>(batch.vertices.size());
        stats.indices += static_cast<uint32_t>(batch.indices.size());

        const glm::vec4& clip = batch.scissor;
        if (clip.x > 0.5f || clip.y > 0.5f ||
            clip.z < viewport_width - 0.5f || clip.w < viewport_height - 0.5f) {
            ++stats.clipped_batches;
        }
    }

    return stats;
}
} // namespace xel::graphics
