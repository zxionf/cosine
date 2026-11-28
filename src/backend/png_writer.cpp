// PNG 写出：与渲染后端无关，只依赖 stb_image_write。
// 放在这里而不是 Renderer 里，是为了让「导出图片」不绑死某个后端实现。
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "backend/render_backend.hpp"

#include <cstddef>

namespace xel::backend
{
bool write_png(const std::string& path, std::uint32_t width, std::uint32_t height,
               const std::vector<unsigned char>& rgba)
{
    if (width == 0 || height == 0) return false;
    const auto expected = static_cast<std::size_t>(width) * height * 4;
    if (rgba.size() < expected) return false;

    return stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4,
                          rgba.data(), static_cast<int>(width) * 4) != 0;
}
} // namespace xel::backend
