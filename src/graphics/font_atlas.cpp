#include "font_atlas.hpp"

#include <algorithm>
#include <string>

namespace
{
xel::graphics::FontAtlas* g_active_font_atlas = nullptr;
}

namespace xel::graphics
{
void FontAtlas::load(int tex_width, int tex_height, const unsigned char* rgba)
{
    tex_width_ = tex_width;
    tex_height_ = tex_height;
    cell_px_ = tex_width / 16;

    for (int idx = 0; idx < 256; ++idx) {
        const int col = idx % 16;
        const int row = idx / 16;
        const int ox = col * cell_px_;
        const int oy = row * cell_px_;

        int min_x = cell_px_;
        int max_x = -1;
        for (int py = 0; py < cell_px_; ++py) {
            for (int px = 0; px < cell_px_; ++px) {
                const int pidx = ((oy + py) * tex_width + (ox + px)) * 4;
                if (rgba[pidx + 3] > 0) {
                    min_x = std::min(min_x, px);
                    max_x = std::max(max_x, px);
                }
            }
        }
        glyphs_[idx] = {min_x, max_x};
    }

    white_uv_ = {0.5f / static_cast<float>(tex_width), 0.5f / static_cast<float>(tex_height)};
    for (int y = 0; y < tex_height; ++y) {
        for (int x = 0; x < tex_width; ++x) {
            const int pidx = (y * tex_width + x) * 4;
            if (rgba[pidx] == 255 && rgba[pidx + 1] == 255 && rgba[pidx + 2] == 255 && rgba[pidx + 3] == 255) {
                white_uv_ = {
                    (static_cast<float>(x) + 0.5f) / static_cast<float>(tex_width),
                    (static_cast<float>(y) + 0.5f) / static_cast<float>(tex_height)
                };
                return;
            }
        }
    }
}

float FontAtlas::advance_for(unsigned char ch, float size) const
{
    if (ch == ' ') return size * 0.5f;
    const Glyph& g = glyphs_[ch];
    if (g.max_x < g.min_x) return 0.0f;
    return size * static_cast<float>(g.max_x - g.min_x + 2) / static_cast<float>(cell_px_);
}

glm::vec2 FontAtlas::measure(std::string_view text, float size) const
{
    float max_w = 0.0f;
    float line_w = 0.0f;
    int lines = 1;

    for (char c : text) {
        if (c == '\n') {
            max_w = std::max(max_w, line_w);
            line_w = 0.0f;
            ++lines;
            continue;
        }
        line_w += advance_for(static_cast<unsigned char>(c), size);
    }
    max_w = std::max(max_w, line_w);
    return {max_w, static_cast<float>(lines) * size};
}

float FontAtlas::measure_width(std::string_view text, float size) const
{
    return measure(text, size).x;
}

void FontAtlas::append_text(DrawBatch& batch, float x, float y, float size,
                            std::string_view text, glm::vec4 color) const
{
    if (!valid()) return;

    batch.texture_slot = TextureSlot::Texture;

    constexpr float cell_uv = 1.0f / 16.0f;
    float pen_x = x;
    float pen_y = y;

    for (char c : text) {
        if (c == '\n') {
            pen_x = x;
            pen_y += size;
            continue;
        }
        if (c == ' ') {
            pen_x += size * 0.5f;
            continue;
        }

        const unsigned char index = static_cast<unsigned char>(c);
        const Glyph& g = glyphs_[index];
        if (g.max_x < g.min_x) continue;

        const int col = index % 16;
        const int row = index / 16;
        const float u_cell = static_cast<float>(col) * cell_uv;
        const float v_cell = static_cast<float>(row) * cell_uv;

        const float u0 = u_cell + (static_cast<float>(g.min_x) / 16.0f) / static_cast<float>(cell_px_);
        const float u1 = u_cell + ((static_cast<float>(g.max_x) + 1.0f) / 16.0f) / static_cast<float>(cell_px_);
        const float v0 = v_cell;
        const float v1 = v_cell + cell_uv;

        const float glyph_w = size * static_cast<float>(g.max_x - g.min_x + 1) / static_cast<float>(cell_px_);
        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());

        batch.vertices.push_back({{pen_x,          pen_y},          color, {u0, v0}});
        batch.vertices.push_back({{pen_x + glyph_w, pen_y},          color, {u1, v0}});
        batch.vertices.push_back({{pen_x + glyph_w, pen_y + size},   color, {u1, v1}});
        batch.vertices.push_back({{pen_x,          pen_y + size},   color, {u0, v1}});

        batch.indices.insert(batch.indices.end(), {
            base, base + 1, base + 2,
            base + 2, base + 3, base
        });

        pen_x += advance_for(index, size);
    }
}

FontAtlas* active_font_atlas()
{
    return g_active_font_atlas;
}

void set_active_font_atlas(FontAtlas* atlas)
{
    g_active_font_atlas = atlas;
}
} // namespace xel::graphics
