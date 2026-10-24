#pragma once

#include "drawable.hpp"

#include <string_view>
#include <unordered_map>

namespace xel::graphics
{
struct TextClip
{
    float left = -1.0e9f;
    float top = -1.0e9f;
    float right = 1.0e9f;
    float bottom = 1.0e9f;
};

class FontAtlas
{
public:
    struct Glyph
    {
        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 0.0f;
        float v1 = 0.0f;
        float advance_ratio = 0.5f;
        float width_ratio = 0.5f;
    };

    void load(int tex_width, int tex_height, const unsigned char* rgba);
    bool valid() const { return tex_width_ > 0 && tex_height_ > 0; }

    int cell_px() const { return cell_px_; }
    glm::vec2 white_uv() const { return white_uv_; }

    bool has_glyph(char32_t codepoint) const;
    const Glyph* glyph(char32_t codepoint) const;

    glm::vec2 measure(std::string_view text, float size) const;
    float measure_width(std::string_view text, float size) const;
    glm::vec2 measure_wrapped(std::string_view text, float size, float max_width) const;

    void append_text(DrawBatch& batch, float x, float y, float size,
                     std::string_view text, glm::vec4 color,
                     const TextClip& clip = {}) const;

    void append_text_wrapped(DrawBatch& batch, float x, float y, float size,
                             std::string_view text, glm::vec4 color,
                             float max_width, int max_lines = 0, bool ellipsis = false,
                             const TextClip& clip = {}) const;

private:
    const Glyph* glyph_or_fallback(char32_t codepoint) const;
    void append_glyph(DrawBatch& batch, float& pen_x, float& pen_y, float size,
                      char32_t codepoint, glm::vec4 color, const TextClip& clip) const;

    int tex_width_ = 0;
    int tex_height_ = 0;
    int cell_px_ = 8;
    glm::vec2 white_uv_{0.0f, 0.0f};
    std::unordered_map<char32_t, Glyph> glyphs_;
};

FontAtlas* active_font_atlas();
void set_active_font_atlas(FontAtlas* atlas);
} // namespace xel::graphics
