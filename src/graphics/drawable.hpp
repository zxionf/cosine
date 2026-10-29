#pragma once

#include "vertex.hpp"

#include <algorithm>
#include <vector>

namespace xel::graphics
{
enum class TextureSlot : uint32_t
{
    White = 0,   // 1×1 白，纯色图元用它
    Texture = 1, // 字体图集（历史命名，保留以免改动调用点）
};

// 多纹理槽（CO-2）：
//   0        White（1×1 白）
//   1        字体图集
//   2..15    用户图片，最多 14 张同时在屏
// 槽号是绘制时的离散维度：同一槽 + 同一裁剪 → 同一批。
constexpr uint32_t kMaxTextureSlots = 16;
constexpr uint32_t kFirstImageSlot = 2;
constexpr uint32_t kMaxImageSlots = kMaxTextureSlots - kFirstImageSlot;

// 图片索引（0 基）→ 纹理槽号。
constexpr uint32_t image_slot(uint32_t index)
{
    return kFirstImageSlot + (index < kMaxImageSlots ? index : kMaxImageSlots - 1);
}

constexpr bool is_image_slot(uint32_t slot)
{
    return slot >= kFirstImageSlot && slot < kMaxTextureSlots;
}

struct DrawBatch
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    uint32_t texture_slot = static_cast<uint32_t>(TextureSlot::White);
    glm::vec4 scissor{0.0f, 0.0f, 0.0f, 0.0f};
    // 建立时的累积透明度。预乘 alpha 下四个通道同乘，就是整棵子树淡出的正确结果。
    float opacity = 1.0f;

    void add_vertex(glm::vec2 pos, glm::vec4 color, glm::vec2 uv = {0.0f, 0.0f})
    {
        if (opacity < 1.0f) {
            color = {color.r * opacity, color.g * opacity, color.b * opacity, color.a * opacity};
        }
        vertices.push_back(Vertex{pos, color, uv});
    }
};

class DrawList
{
public:
    void clear()
    {
        batches_.clear();
        clips_.clear();
        clips_.push_back(full_clip_);
        texture_slot_ = static_cast<uint32_t>(TextureSlot::White);
        opacity_ = 1.0f;
        opacity_stack_.clear();
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
        // 裁剪边界对齐到像素网格：渲染层按整数 scissor 提交，
        // 两边都取整才不会让控件顶部/底部多出一行像素。
        clip = {std::round(clip.x), std::round(clip.y), std::round(clip.z), std::round(clip.w)};
        if (clips_.empty()) clips_.push_back(clip);
        else clips_.back() = clip;
        needs_new_batch_ = true;
    }

    void push_clip(glm::vec4 clip)
    {
        clip = {std::round(clip.x), std::round(clip.y), std::round(clip.z), std::round(clip.w)};
        clips_.push_back(intersect(current_clip(), clip));
        needs_new_batch_ = true;
    }

    void pop_clip()
    {
        if (clips_.size() > 1) clips_.pop_back();
        needs_new_batch_ = true;
    }

    // 透明度乘子：影响此后写入的所有顶点（文字、色块、贴图、线段都走顶点色）。
    // 支持嵌套，内层与外层相乘。
    void push_opacity(float value)
    {
        opacity_stack_.push_back(opacity_);
        opacity_ *= std::clamp(value, 0.0f, 1.0f);
        needs_new_batch_ = true;
    }
    void pop_opacity()
    {
        if (opacity_stack_.empty()) return;
        opacity_ = opacity_stack_.back();
        opacity_stack_.pop_back();
        needs_new_batch_ = true;
    }
    float opacity() const { return opacity_; }

    glm::vec4 current_clip() const
    {
        return clips_.empty() ? full_clip_ : clips_.back();
    }

    void set_texture(TextureSlot slot) { set_texture_slot(static_cast<uint32_t>(slot)); }

    // 图片槽用槽号直接指定（见 kFirstImageSlot）。
    void set_texture_slot(uint32_t slot)
    {
        if (texture_slot_ != slot) {
            texture_slot_ = slot;
            needs_new_batch_ = true;
        }
    }

    uint32_t texture_slot() const { return texture_slot_; }

    DrawBatch& batch()
    {
        if (batches_.empty() || needs_new_batch_) {
            if (!batches_.empty() && batches_.back().vertices.empty()) {
                batches_.back().texture_slot = texture_slot_;
                batches_.back().scissor = current_clip();
                batches_.back().opacity = opacity_;
            } else {
                DrawBatch batch;
                batch.texture_slot = texture_slot_;
                batch.scissor = current_clip();
                batch.opacity = opacity_;
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
    uint32_t texture_slot_ = static_cast<uint32_t>(TextureSlot::White);
    bool needs_new_batch_ = true;
    float opacity_ = 1.0f;
    std::vector<float> opacity_stack_;
};

class Drawable
{
public:
    virtual ~Drawable() = default;
    virtual void append_to(DrawList& list) const = 0;
    virtual TextureSlot texture_slot() const { return TextureSlot::White; }
};
} // namespace xel::graphics
