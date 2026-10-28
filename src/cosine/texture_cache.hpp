#pragma once

#include "backend/vulkan/renderer.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace xel::editor
{
// 图片资源缓存（CO-2）：路径 → 图片槽索引。
// 同路径只上传一次；槽满（14 张）时拒绝新图并返回 0，调用方回退到色块。
//
// 返回值统一为「图片槽索引 + 1」，与 `scene::Node::texture_id` 的语义一致：
// 0 = 未绑定。
class TextureCache
{
public:
    std::uint32_t load(const std::string& path, backend::vulkan::Renderer& renderer);
    // 路径已缓存则返回其句柄，否则返回 0（不触发上传）。
    std::uint32_t find(const std::string& path) const;
    void clear();

    std::uint32_t size() const { return static_cast<std::uint32_t>(by_path_.size()); }
    std::uint32_t capacity() const { return backend::vulkan::Renderer::max_image_slots(); }

private:
    struct Entry
    {
        std::uint32_t slot = 0; // 图片槽索引（0 基）
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    std::unordered_map<std::string, Entry> by_path_;
    std::uint32_t next_slot_ = 0;
};
} // namespace xel::editor
