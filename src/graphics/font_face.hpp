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

    bool add_glyph(FontAtlas& atlas, char32_t codepoint) const;

private:
    FT_Library library_ = nullptr;
    FT_Face face_ = nullptr;
    int pixel_size_ = 16;
};
} // namespace xel::graphics
