#include "font_face.hpp"

namespace xel::graphics
{
int FontFace::set_pixel_size(int pixel_size) const
{
    if (!face_ || pixel_size <= 0) return -1;
    return FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(pixel_size));
}

FontFace::~FontFace()
{
    if (face_) FT_Done_Face(face_);
    if (library_) FT_Done_FreeType(library_);
}

bool FontFace::load(const std::string& path, int face_index, int pixel_size)
{
    if (path.empty() || pixel_size <= 0) return false;

    if (FT_Init_FreeType(&library_) != 0) return false;
    if (FT_New_Face(library_, path.c_str(), face_index, &face_) != 0) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
        face_ = nullptr;
        return false;
    }
    if (set_pixel_size(pixel_size) != 0) {
        FT_Done_Face(face_);
        FT_Done_FreeType(library_);
        face_ = nullptr;
        library_ = nullptr;
        return false;
    }

    pixel_size_ = pixel_size;
    base_metrics_ = metrics();
    return true;
}

FontAtlas::Metrics FontFace::metrics() const
{
    FontAtlas::Metrics metrics;
    if (!face_) return metrics;

    const float pixel_size = static_cast<float>(pixel_size_);
    metrics.ascent_ratio = static_cast<float>(face_->size->metrics.ascender) / 64.0f / pixel_size;
    metrics.descent_ratio = static_cast<float>(face_->size->metrics.descender) / 64.0f / pixel_size;
    metrics.line_height_ratio = static_cast<float>(face_->size->metrics.height) / 64.0f / pixel_size;
    return FontAtlas::normalize_metrics(metrics);
}

bool FontFace::add_glyph(FontAtlas& atlas, char32_t codepoint, int pixel_size) const
{
    if (!face_) return false;
    const int target = pixel_size > 0 ? pixel_size : pixel_size_;
    if (target != pixel_size_) {
        if (set_pixel_size(target) != 0) return false;
    }
    if (FT_Load_Char(face_, codepoint, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) != 0) return false;

    const FT_GlyphSlot slot = face_->glyph;
    if (slot->glyph_index == 0) return false;

    const float px = static_cast<float>(target);
    const float advance_ratio = static_cast<float>(slot->advance.x) / 64.0f / px;
    const FT_Bitmap& bitmap = slot->bitmap;

    if (bitmap.width == 0 || bitmap.rows == 0) {
        atlas.add_empty_glyph(codepoint, advance_ratio, px);
        return true;
    }

    return atlas.add_glyph(codepoint,
                           bitmap.buffer,
                           static_cast<int>(bitmap.width),
                           static_cast<int>(bitmap.rows),
                           static_cast<int>(bitmap.pitch),
                           advance_ratio,
                           static_cast<float>(slot->bitmap_left) / px,
                           static_cast<float>(slot->bitmap_top) / px,
                           px);
}
} // namespace xel::graphics
