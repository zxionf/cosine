#include "font_atlas.hpp"
#include "utf8.hpp"

#include <algorithm>
#include <cstring>
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
    cell_px_ = std::max(1, tex_width / 16);
    smooth_ = false;
    metrics_ = {};
    glyphs_.clear();
    missing_.clear();
    glyph_loader_ = nullptr;

    pixels_.assign(rgba, rgba + static_cast<std::size_t>(tex_width) * tex_height * 4);

    constexpr float cell_uv = 1.0f / 16.0f;

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

        Glyph g;
        g.u0 = static_cast<float>(col) * cell_uv;
        g.v0 = static_cast<float>(row) * cell_uv;
        g.u1 = g.u0 + cell_uv;
        g.v1 = g.v0 + cell_uv;

        if (max_x >= min_x) {
            g.u0 += (static_cast<float>(min_x) / 16.0f) / static_cast<float>(cell_px_);
            g.u1 = static_cast<float>(col) * cell_uv
                 + ((static_cast<float>(max_x) + 1.0f) / 16.0f) / static_cast<float>(cell_px_);
            g.width_ratio = static_cast<float>(max_x - min_x + 1) / static_cast<float>(cell_px_);
            g.height_ratio = 1.0f;
            g.bearing_x_ratio = static_cast<float>(min_x) / static_cast<float>(cell_px_);
            g.bearing_y_ratio = 1.0f;
            g.advance_ratio = static_cast<float>(max_x - min_x + 2) / static_cast<float>(cell_px_);
        } else {
            g.width_ratio = 0.0f;
            g.advance_ratio = (idx == ' ') ? 0.5f : 0.0f;
        }

        glyphs_.emplace(static_cast<char32_t>(idx), g);
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
                break;
            }
        }
    }

    mark_revision();
}

void FontAtlas::begin_generated(int tex_size, Metrics metrics, bool smooth)
{
    tex_width_ = tex_size;
    tex_height_ = tex_size;
    cell_px_ = 0;
    smooth_ = smooth;
    metrics_ = metrics;
    glyphs_.clear();
    missing_.clear();
    glyph_loader_ = nullptr;
    pixels_.assign(static_cast<std::size_t>(tex_size) * tex_size * 4, 0);

    // 左上角保留一个纯白像素，供矩形/线段采样。
    pixels_[0] = 255;
    pixels_[1] = 255;
    pixels_[2] = 255;
    pixels_[3] = 255;
    white_uv_ = {0.5f / static_cast<float>(tex_size), 0.5f / static_cast<float>(tex_size)};

    pack_x_ = 1;
    pack_y_ = 1;
    pack_row_height_ = 0;
    mark_revision();
}

bool FontAtlas::add_glyph(char32_t codepoint,
                          const unsigned char* bitmap, int width, int height, int pitch,
                          float advance_ratio, float bearing_x_ratio, float bearing_y_ratio,
                          float pixel_size)
{
    if (width <= 0 || height <= 0) {
        add_empty_glyph(codepoint, advance_ratio);
        return true;
    }

    if (pack_x_ + width + 2 >= tex_width_) {
        pack_x_ = 1;
        pack_y_ += pack_row_height_ + 2;
        pack_row_height_ = 0;
    }
    if (pack_y_ + height + 2 >= tex_height_) return false;

    const int dst_x = pack_x_ + 1;
    const int dst_y = pack_y_ + 1;

    for (int y = 0; y < height; ++y) {
        const unsigned char* src = bitmap + y * pitch;
        unsigned char* dst = pixels_.data() + (static_cast<std::size_t>(dst_y + y) * tex_width_ + dst_x) * 4;
        for (int x = 0; x < width; ++x) {
            const unsigned char alpha = src[x];
            // 预乘 alpha：与当前 One / OneMinusSrcAlpha 的混合状态匹配。
            dst[x * 4 + 0] = alpha;
            dst[x * 4 + 1] = alpha;
            dst[x * 4 + 2] = alpha;
            dst[x * 4 + 3] = alpha;
        }
    }

    Glyph g;
    g.u0 = static_cast<float>(dst_x) / static_cast<float>(tex_width_);
    g.v0 = static_cast<float>(dst_y) / static_cast<float>(tex_height_);
    g.u1 = static_cast<float>(dst_x + width) / static_cast<float>(tex_width_);
    g.v1 = static_cast<float>(dst_y + height) / static_cast<float>(tex_height_);
    g.advance_ratio = advance_ratio;
    g.width_ratio = static_cast<float>(width) / pixel_size;
    g.height_ratio = static_cast<float>(height) / pixel_size;
    g.bearing_x_ratio = bearing_x_ratio;
    g.bearing_y_ratio = bearing_y_ratio;
    glyphs_[codepoint] = g;

    pack_x_ += width + 2;
    pack_row_height_ = std::max(pack_row_height_, height);
    mark_revision();
    return true;
}

void FontAtlas::add_empty_glyph(char32_t codepoint, float advance_ratio)
{
    Glyph g;
    g.advance_ratio = advance_ratio;
    glyphs_[codepoint] = g;
    mark_revision();
}

bool FontAtlas::ensure_glyph(char32_t codepoint)
{
    if (has_glyph(codepoint)) return true;
    if (missing_.find(codepoint) != missing_.end()) return false;
    if (!glyph_loader_) return false;

    if (glyph_loader_(*this, codepoint)) return true;
    missing_.insert(codepoint);
    return false;
}

bool FontAtlas::has_glyph(char32_t codepoint) const
{
    return glyphs_.find(codepoint) != glyphs_.end();
}

const FontAtlas::Glyph* FontAtlas::glyph(char32_t codepoint) const
{
    const auto it = glyphs_.find(codepoint);
    return it == glyphs_.end() ? nullptr : &it->second;
}

const FontAtlas::Glyph* FontAtlas::glyph_or_fallback(char32_t codepoint) const
{
    if (const Glyph* g = glyph(codepoint)) return g;
    if (const Glyph* g = glyph(U'?')) return g;
    return nullptr;
}

glm::vec2 FontAtlas::measure(std::string_view text, float size, float line_spacing)
{
    float max_w = 0.0f;
    float line_w = 0.0f;
    int lines = 1;

    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t codepoint = utf8_decode(text, index);
        if (codepoint == U'\n') {
            max_w = std::max(max_w, line_w);
            line_w = 0.0f;
            ++lines;
            continue;
        }
        if (codepoint == U'\r') continue;
        ensure_glyph(codepoint);
        const Glyph* g = glyph_or_fallback(codepoint);
        line_w += (g ? g->advance_ratio : 0.5f) * size;
    }

    max_w = std::max(max_w, line_w);
    const float height = static_cast<float>(lines) * line_height(size)
        + static_cast<float>(lines - 1) * std::max(0.0f, line_spacing);
    return {max_w, height};
}

float FontAtlas::measure_width(std::string_view text, float size)
{
    return measure(text, size).x;
}

glm::vec2 FontAtlas::measure_wrapped(std::string_view text, float size, float max_width,
                                     float line_spacing)
{
    if (max_width <= 0.0f) return measure(text, size, line_spacing);

    float max_w = 0.0f;
    float line_w = 0.0f;
    int lines = 1;

    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t codepoint = utf8_decode(text, index);
        if (codepoint == U'\n') {
            max_w = std::max(max_w, line_w);
            line_w = 0.0f;
            ++lines;
            continue;
        }
        if (codepoint == U'\r') continue;

        ensure_glyph(codepoint);
        const Glyph* g = glyph_or_fallback(codepoint);
        const float advance = (g ? g->advance_ratio : 0.5f) * size;
        if (line_w > 0.0f && line_w + advance > max_width) {
            max_w = std::max(max_w, line_w);
            line_w = 0.0f;
            ++lines;
        }
        line_w += advance;
    }

    max_w = std::max(max_w, line_w);
    const float height = static_cast<float>(lines) * line_height(size)
        + static_cast<float>(lines - 1) * std::max(0.0f, line_spacing);
    return {std::min(max_w, max_width), height};
}

void FontAtlas::append_glyph(DrawBatch& batch, float& pen_x, float& pen_y, float size,
                             char32_t codepoint, glm::vec4 color, const TextClip& clip) const
{
    const Glyph* g = glyph_or_fallback(codepoint);
    if (!g) {
        pen_x += size * 0.5f;
        return;
    }

    const float advance = g->advance_ratio * size;
    if (g->width_ratio <= 0.0f || g->height_ratio <= 0.0f) {
        pen_x += advance;
        return;
    }

    const float baseline = pen_y + metrics_.ascent_ratio * size;
    const float x0 = pen_x + g->bearing_x_ratio * size;
    const float y0 = baseline - g->bearing_y_ratio * size;
    const float x1 = x0 + g->width_ratio * size;
    const float y1 = y0 + g->height_ratio * size;

    const float cx0 = std::max(x0, clip.left);
    const float cy0 = std::max(y0, clip.top);
    const float cx1 = std::min(x1, clip.right);
    const float cy1 = std::min(y1, clip.bottom);

    if (cx1 > cx0 && cy1 > cy0) {
        const float inv_w = 1.0f / (x1 - x0);
        const float inv_h = 1.0f / (y1 - y0);
        const float u0 = g->u0 + (cx0 - x0) * inv_w * (g->u1 - g->u0);
        const float u1 = g->u0 + (cx1 - x0) * inv_w * (g->u1 - g->u0);
        const float v0 = g->v0 + (cy0 - y0) * inv_h * (g->v1 - g->v0);
        const float v1 = g->v0 + (cy1 - y0) * inv_h * (g->v1 - g->v0);

        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.vertices.push_back({{cx0, cy0}, color, {u0, v0}});
        batch.vertices.push_back({{cx1, cy0}, color, {u1, v0}});
        batch.vertices.push_back({{cx1, cy1}, color, {u1, v1}});
        batch.vertices.push_back({{cx0, cy1}, color, {u0, v1}});
        batch.indices.insert(batch.indices.end(), {
            base, base + 1, base + 2,
            base + 2, base + 3, base
        });
    }

    pen_x += advance;
}

void FontAtlas::append_text(DrawList& list, float x, float y, float size,
                            std::string_view text, glm::vec4 color,
                            const TextClip& clip, float line_spacing)
{
    if (!valid()) return;

    list.set_texture(TextureSlot::Texture);
    auto& batch = list.batch();
    float pen_x = x;
    float pen_y = y;

    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t codepoint = utf8_decode(text, index);
        if (codepoint == U'\n') {
            pen_x = x;
            pen_y += line_step(size, line_spacing);
            continue;
        }
        if (codepoint == U'\r') continue;
        ensure_glyph(codepoint);
        append_glyph(batch, pen_x, pen_y, size, codepoint, color, clip);
    }
}

void FontAtlas::append_text_wrapped(DrawList& list, float x, float y, float size,
                                    std::string_view text, glm::vec4 color,
                                    float max_width, int max_lines, bool ellipsis,
                                    const TextClip& clip, float line_spacing)
{
    if (!valid() || max_width <= 0.0f) return;

    list.set_texture(TextureSlot::Texture);
    auto& batch = list.batch();
    float pen_x = x;
    float pen_y = y;
    float line_w = 0.0f;
    int lines_used = 1;

    auto draw_ellipsis = [&]() {
        if (!ellipsis) return;
        for (int i = 0; i < 3; ++i) {
            const Glyph* dot = glyph_or_fallback(U'.');
            if (!dot) break;
            const float advance = dot->advance_ratio * size;
            if (line_w + advance > max_width) break;
            append_glyph(batch, pen_x, pen_y, size, U'.', color, clip);
            line_w += advance;
        }
    };

    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t codepoint = utf8_decode(text, index);

        if (codepoint == U'\r') continue;
        if (codepoint == U'\n') {
            if (max_lines > 0 && lines_used >= max_lines) {
                draw_ellipsis();
                return;
            }
            pen_x = x;
            pen_y += line_step(size, line_spacing);
            line_w = 0.0f;
            ++lines_used;
            continue;
        }

        ensure_glyph(codepoint);
        const Glyph* g = glyph_or_fallback(codepoint);
        const float advance = (g ? g->advance_ratio : 0.5f) * size;

        if (line_w > 0.0f && line_w + advance > max_width) {
            if (max_lines > 0 && lines_used >= max_lines) {
                draw_ellipsis();
                return;
            }
            pen_x = x;
            pen_y += line_step(size, line_spacing);
            line_w = 0.0f;
            ++lines_used;
        }

        append_glyph(batch, pen_x, pen_y, size, codepoint, color, clip);
        line_w += advance;
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
