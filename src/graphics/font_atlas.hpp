#pragma once

#include "drawable.hpp"

#include <cstdint>
#include <functional>
#include <limits>
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

    using GlyphLoader = std::function<bool(FontAtlas&, char32_t, int pixel_size)>;

    // 保证 line_height 至少覆盖 ascender 到 descender 的完整字形高度。
    // descent_ratio 按 FreeType 约定为负值。
    static Metrics normalize_metrics(Metrics metrics);

    void load(int tex_width, int tex_height, const unsigned char* rgba);
    void begin_generated(int tex_size, Metrics metrics, bool smooth, int base_pixel_size = 16);

    bool add_glyph(char32_t codepoint,
                   const unsigned char* bitmap, int width, int height, int pitch,
                   float advance_ratio, float bearing_x_ratio, float bearing_y_ratio,
                   float pixel_size);
    void add_empty_glyph(char32_t codepoint, float advance_ratio, float pixel_size = 16.0f);

    void set_glyph_loader(GlyphLoader loader) { glyph_loader_ = std::move(loader); }

    // 按整数像素量化字号：字形按绘制尺寸栅格化，避免放大 16px 基准图集导致模糊。
    static int quantize_size(float size);

    bool ensure_glyph(char32_t codepoint, int pixel_size);
    bool ensure_glyph(char32_t codepoint) { return ensure_glyph(codepoint, base_pixel_size_); }
    int base_pixel_size() const { return base_pixel_size_; }

    // DPI：字形按「逻辑字号 × pixel_ratio」栅格化，绘制仍用逻辑字号（由投影放大）。
    void set_pixel_ratio(float ratio) { pixel_ratio_ = ratio > 0.0f ? ratio : 1.0f; }
    float pixel_ratio() const { return pixel_ratio_; }
    int glyph_pixel_size(float size) const { return quantize_size(size * pixel_ratio_); }

    bool valid() const { return tex_width_ > 0 && tex_height_ > 0; }
    int width() const { return tex_width_; }
    int height() const { return tex_height_; }
    int cell_px() const { return cell_px_; }
    bool smooth() const { return smooth_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<unsigned char>& pixels() const { return pixels_; }
    glm::vec2 white_uv() const { return white_uv_; }
    uint64_t revision() const { return revision_; }

    // 已用高度占比（shelf packing 进度），供监控与测试。
    float used_ratio() const;
    // 最近一次 add_glyph 是否因图集空间不足失败（区别于「字符不存在」）。
    bool last_add_out_of_space() const { return last_add_out_of_space_; }

    // 取出自上次调用以来的脏矩形（含端点）；无像素变化返回 false 并清零。
    bool take_dirty_rect(int& x, int& y, int& w, int& h);

    float line_height(float size) const { return metrics_.line_height_ratio * size; }
    float line_step(float size, float line_spacing) const { return line_height(size) + line_spacing; }

    bool has_glyph(char32_t codepoint, int pixel_size) const;
    bool has_glyph(char32_t codepoint) const { return has_glyph(codepoint, base_pixel_size_); }
    const Glyph* glyph(char32_t codepoint, int pixel_size) const;
    const Glyph* glyph(char32_t codepoint) const { return glyph(codepoint, base_pixel_size_); }

    glm::vec2 measure(std::string_view text, float size, float line_spacing = 0.0f);
    float measure_width(std::string_view text, float size);
    glm::vec2 measure_wrapped(std::string_view text, float size, float max_width,
                              float line_spacing = 0.0f);

    void append_text(DrawList& list, float x, float y, float size,
                     std::string_view text, glm::vec4 color,
                     const TextClip& clip = {}, float line_spacing = 0.0f);

    void append_text_wrapped(DrawList& list, float x, float y, float size,
                             std::string_view text, glm::vec4 color,
                             float max_width, int max_lines = 0, bool ellipsis = false,
                             const TextClip& clip = {}, float line_spacing = 0.0f);

private:
    const Glyph* glyph_or_fallback(char32_t codepoint, int pixel_size) const;
    static uint64_t glyph_key(char32_t codepoint, int pixel_size);
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
    std::unordered_map<uint64_t, Glyph> glyphs_;
    std::unordered_set<uint64_t> missing_;
    int base_pixel_size_ = 16;
    float pixel_ratio_ = 1.0f;
    GlyphLoader glyph_loader_;
    uint64_t revision_ = 0;
    bool last_add_out_of_space_ = false;
    int dirty_min_x_ = std::numeric_limits<int>::max();
    int dirty_min_y_ = std::numeric_limits<int>::max();
    int dirty_max_x_ = -1;
    int dirty_max_y_ = -1;

    int pack_x_ = 1;
    int pack_y_ = 1;
    int pack_row_height_ = 0;
};

FontAtlas* active_font_atlas();
void set_active_font_atlas(FontAtlas* atlas);
} // namespace xel::graphics
