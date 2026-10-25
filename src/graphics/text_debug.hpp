#pragma once

#include "font_atlas.hpp"
#include "utf8.hpp"
#include "rectangle.hpp"
#include "line.hpp"

#include <algorithm>
#include <string_view>

namespace xel::graphics
{
struct TextDebugOptions
{
    bool glyph_boxes = false;   // 每个字形的包围盒
    bool line_box = false;      // 整段文本的包围盒
    bool baseline = false;      // 基线
    bool advance = false;       // advance 刻度线
    glm::vec4 color{0.20f, 1.00f, 0.85f, 0.85f};
    float thickness = 1.0f;
    float line_spacing = 0.0f;  // 与 Label 一致的行距
    float wrap_width = 0.0f;    // >0 时按此宽度换行，与 Label 的换行逻辑一致
    int max_lines = 0;          // 0 = 不限

    bool any() const { return glyph_boxes || line_box || baseline || advance; }
};

namespace detail
{
inline void append_outline(DrawList& list, float x, float y, float w, float h,
                           glm::vec4 color, float t)
{
    Rectangle top{x, y, w, t, color};
    Rectangle bottom{x, y + std::max(0.0f, h - t), w, t, color};
    Rectangle left{x, y, t, h, color};
    Rectangle right{x + std::max(0.0f, w - t), y, t, h, color};
    top.append_to(list);
    bottom.append_to(list);
    left.append_to(list);
    right.append_to(list);
}
} // namespace detail

// 在已绘制文本之上叠加调试几何；绘制数学与 FontAtlas::append_glyph 保持同源。
inline void append_text_debug(DrawList& list, FontAtlas& atlas,
                              float x, float y, float size,
                              std::string_view text,
                              const TextDebugOptions& options)
{
    if (!options.any() || !atlas.valid()) return;

    const auto& metrics = atlas.metrics();
    const float step = atlas.line_step(size, options.line_spacing);
    const float ascent = metrics.ascent_ratio * size;
    const float t = options.thickness;

    float pen_x = x;
    float pen_y = y;
    float line_begin_x = x;
    float line_extent_x = x;
    int lines_used = 1;

    auto flush_line = [&] {
        if (options.line_box) {
            detail::append_outline(list, line_begin_x, pen_y,
                                   std::max(1.0f, line_extent_x - line_begin_x),
                                   atlas.line_height(size), options.color, t);
        }
        if (options.baseline) {
            Line line{line_begin_x, pen_y + ascent, std::max(line_extent_x, pen_x),
                      pen_y + ascent, t, options.color};
            line.append_to(list);
        }
    };

    auto new_line = [&] {
        flush_line();
        pen_x = x;
        pen_y += step;
        line_begin_x = x;
        line_extent_x = x;
        ++lines_used;
    };

    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t codepoint = utf8_decode(text, index);
        if (codepoint == U'\r') continue;
        if (codepoint == U'\n') {
            new_line();
            continue;
        }

        const FontAtlas::Glyph* glyph = atlas.glyph(codepoint);
        const float advance = (glyph ? glyph->advance_ratio : 0.5f) * size;

        // 换行规则与 append_text_wrapped 一致：行内已有内容且放不下时折行。
        if (options.wrap_width > 0.0f && pen_x > line_begin_x
            && pen_x + advance > x + options.wrap_width) {
            if (options.max_lines > 0 && lines_used >= options.max_lines) break;
            new_line();
        }

        if (glyph && glyph->width_ratio > 0.0f && glyph->height_ratio > 0.0f) {
            const float baseline_y = pen_y + ascent;
            const float gx0 = pen_x + glyph->bearing_x_ratio * size;
            const float gy0 = baseline_y - glyph->bearing_y_ratio * size;
            const float gw = glyph->width_ratio * size;
            const float gh = glyph->height_ratio * size;

            if (options.glyph_boxes) {
                detail::append_outline(list, gx0, gy0, gw, gh, options.color, t);
            }
            line_extent_x = std::max(line_extent_x, gx0 + gw);
        }

        if (options.advance) {
            const float baseline_y = pen_y + ascent;
            Line tick{pen_x, baseline_y - size * 0.15f, pen_x, baseline_y + size * 0.85f,
                      t, options.color};
            tick.append_to(list);
        }

        pen_x += advance;
        line_extent_x = std::max(line_extent_x, pen_x);
    }

    flush_line();
    if (options.advance) {
        Line tick{pen_x, pen_y + ascent - size * 0.15f, pen_x, pen_y + ascent + size * 0.85f,
                  t, options.color};
        tick.append_to(list);
    }
}
} // namespace xel::graphics
