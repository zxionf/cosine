#pragma once

#include "drawable.hpp"

#include <array>
#include <string_view>

namespace xel::graphics
{
class FontAtlas
{
public:
    struct Glyph
    {
        int min_x = 0;
        int max_x = -1;
    };

    void load(int tex_width, int tex_height, const unsigned char* rgba);
    bool valid() const { return tex_width_ > 0 && tex_height_ > 0; }

    int cell_px() const { return cell_px_; }
    glm::vec2 white_uv() const { return white_uv_; }
    glm::vec2 measure(std::string_view text, float size) const;
    float measure_width(std::string_view text, float size) const;

    void append_text(DrawBatch& batch, float x, float y, float size,
                     std::string_view text, glm::vec4 color) const;

private:
    float advance_for(unsigned char ch, float size) const;

    int tex_width_ = 0;
    int tex_height_ = 0;
    int cell_px_ = 8;
    std::array<Glyph, 256> glyphs_{};
    glm::vec2 white_uv_{0.0f, 0.0f};
};

FontAtlas* active_font_atlas();
void set_active_font_atlas(FontAtlas* atlas);
} // namespace xel::graphics
