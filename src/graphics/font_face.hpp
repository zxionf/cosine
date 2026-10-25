#pragma once

#include "font_atlas.hpp"

#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace xel::graphics
{
class FontFace
{
public:
    FontFace() = default;
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    bool load(const std::string& path, int face_index, int pixel_size);
    bool valid() const { return face_ != nullptr; }
    int pixel_size() const { return pixel_size_; }
    FontAtlas::Metrics metrics() const;

    // pixel_size = 0 表示沿用 load() 时的基准字号；否则按目标像素尺寸栅格化。
    bool add_glyph(FontAtlas& atlas, char32_t codepoint, int pixel_size = 0) const;
    int set_pixel_size(int pixel_size) const;

private:
    FT_Library library_ = nullptr;
    FT_Face face_ = nullptr;
    int pixel_size_ = 16;
    FontAtlas::Metrics base_metrics_{};
};
} // namespace xel::graphics
