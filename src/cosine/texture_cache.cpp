#include "texture_cache.hpp"

#include <stb_image.h>

namespace xel::editor
{
std::uint32_t TextureCache::load(const std::string& path, backend::vulkan::Renderer& renderer)
{
    if (path.empty()) return 0;
    if (const auto it = by_path_.find(path); it != by_path_.end()) return it->second.slot + 1;
    if (next_slot_ >= renderer.max_image_slots()) return 0;

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) stbi_image_free(pixels);
        return 0;
    }

    const bool ok = renderer.upload_texture(
        next_slot_, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), pixels);
    stbi_image_free(pixels);
    if (!ok) return 0;

    const std::uint32_t slot = next_slot_++;
    by_path_.emplace(path, Entry{slot, static_cast<std::uint32_t>(width),
                                 static_cast<std::uint32_t>(height)});
    return slot + 1;
}

std::uint32_t TextureCache::find(const std::string& path) const
{
    const auto it = by_path_.find(path);
    return it == by_path_.end() ? 0 : it->second.slot + 1;
}

void TextureCache::clear()
{
    // 槽位由 renderer 持有：这里只清映射，不主动回收 GPU 资源。
    // 重新加载项目时会走 load() 重新上传（槽索引从 0 重新分配）。
    by_path_.clear();
    next_slot_ = 0;
}
} // namespace xel::editor
