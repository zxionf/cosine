#pragma once

#include "vertex.hpp"

#include <algorithm>
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

class DrawList
{
public:
    void clear()
    {
        batches_.clear();
        clips_.clear();
        clips_.push_back(full_clip_);
        texture_slot_ = TextureSlot::White;
        needs_new_batch_ = true;
    }

    void set_full_clip(glm::vec4 clip)
    {
        full_clip_ = clip;
        reset_clip();
    }

    void reset_clip()
    {
        clips_.clear();
        clips_.push_back(full_clip_);
        needs_new_batch_ = true;
    }
    void set_clip(glm::vec4 clip)
    {
        if (clips_.empty()) clips_.push_back(clip);
        else clips_.back() = clip;
        needs_new_batch_ = true;
    }

    void push_clip(glm::vec4 clip)
    {
        clips_.push_back(intersect(current_clip(), clip));
        needs_new_batch_ = true;
    }

    void pop_clip()
    {
        if (clips_.size() > 1) clips_.pop_back();
        needs_new_batch_ = true;
    }

    glm::vec4 current_clip() const
    {
        return clips_.empty() ? full_clip_ : clips_.back();
    }

    void set_texture(TextureSlot slot)
    {
        if (texture_slot_ != slot) {
            texture_slot_ = slot;
            needs_new_batch_ = true;
        }
    }

    DrawBatch& batch()
    {
        if (batches_.empty() || needs_new_batch_) {
            if (!batches_.empty() && batches_.back().vertices.empty()) {
                batches_.back().texture_slot = texture_slot_;
                batches_.back().scissor = current_clip();
            } else {
                DrawBatch batch;
                batch.texture_slot = texture_slot_;
                batch.scissor = current_clip();
                batches_.push_back(std::move(batch));
            }
            needs_new_batch_ = false;
        }
        return batches_.back();
    }

    std::vector<DrawBatch>& batches() { return batches_; }
    const std::vector<DrawBatch>& batches() const { return batches_; }

private:
    static glm::vec4 intersect(glm::vec4 a, glm::vec4 b)
    {
        const float x0 = std::max(a.x, b.x);
        const float y0 = std::max(a.y, b.y);
        const float x1 = std::min(a.x + a.z, b.x + b.z);
        const float y1 = std::min(a.y + a.w, b.y + b.w);
        return {x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
    }

    std::vector<DrawBatch> batches_;
    std::vector<glm::vec4> clips_;
    glm::vec4 full_clip_{0.0f, 0.0f, 0.0f, 0.0f};
    TextureSlot texture_slot_ = TextureSlot::White;
    bool needs_new_batch_ = true;
};

class Drawable
{
public:
    virtual ~Drawable() = default;
    virtual void append_to(DrawList& list) const = 0;
    virtual TextureSlot texture_slot() const { return TextureSlot::White; }
};
} // namespace xel::graphics
