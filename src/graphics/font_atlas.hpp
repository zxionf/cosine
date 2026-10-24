#pragma once

#include "drawable.hpp"

#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
        float width_ratio = 0.0f;
        float height_ratio = 0.0f;
        float bearing_x_ratio = 0.0f;
        float bearing_y_ratio = 0.0f;
    };

    struct Metrics
    {
        float ascent_ratio = 1.0f;
        float descent_ratio = 0.0f;
        float line_height_ratio = 1.0f;
    };

    using GlyphLoader = std::function<bool(FontAtlas&, char32_t)>;

    void load(int tex_width, int tex_height, const unsigned char* rgba);
    void begin_generated(int tex_size, Metrics metrics, bool smooth);

    bool add_glyph(char32_t codepoint,
                   const unsigned char* bitmap, int width, int height, int pitch,
                   float advance_ratio, float bearing_x_ratio, float bearing_y_ratio,
                   float pixel_size);
    void add_empty_glyph(char32_t codepoint, float advance_ratio);

    void set_glyph_loader(GlyphLoader loader) { glyph_loader_ = std::move(loader); }
    bool ensure_glyph(char32_t codepoint);

    bool valid() const { return tex_width_ > 0 && tex_height_ > 0; }
    int width() const { return tex_width_; }
    int height() const { return tex_height_; }
    int cell_px() const { return cell_px_; }
    bool smooth() const { return smooth_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<unsigned char>& pixels() const { return pixels_; }
    glm::vec2 white_uv() const { return white_uv_; }
    uint64_t revision() const { return revision_; }

    float line_height(float size) const { return metrics_.line_height_ratio * size; }

    bool has_glyph(char32_t codepoint) const;
    const Glyph* glyph(char32_t codepoint) const;

    glm::vec2 measure(std::string_view text, float size);
    float measure_width(std::string_view text, float size);
    glm::vec2 measure_wrapped(std::string_view text, float size, float max_width);

    void append_text(DrawList& list, float x, float y, float size,
                     std::string_view text, glm::vec4 color,
                     const TextClip& clip = {});

    void append_text_wrapped(DrawList& list, float x, float y, float size,
                             std::string_view text, glm::vec4 color,
                             float max_width, int max_lines = 0, bool ellipsis = false,
                             const TextClip& clip = {});

private:
    const Glyph* glyph_or_fallback(char32_t codepoint) const;
    void append_glyph(DrawBatch& batch, float& pen_x, float& pen_y, float size,
                      char32_t codepoint, glm::vec4 color, const TextClip& clip) const;
    void mark_revision() { ++revision_; }

    int tex_width_ = 0;
    int tex_height_ = 0;
    int cell_px_ = 8;
    bool smooth_ = false;
    glm::vec2 white_uv_{0.0f, 0.0f};
    Metrics metrics_{};
    std::vector<unsigned char> pixels_;
    std::unordered_map<char32_t, Glyph> glyphs_;
    std::unordered_set<char32_t> missing_;
    GlyphLoader glyph_loader_;
    uint64_t revision_ = 0;

    int pack_x_ = 1;
    int pack_y_ = 1;
    int pack_row_height_ = 0;
};

FontAtlas* active_font_atlas();
void set_active_font_atlas(FontAtlas* atlas);
} // namespace xel::graphics
