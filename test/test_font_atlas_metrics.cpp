#include "graphics/font_atlas.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using xel::graphics::FontAtlas;

constexpr float kEps = 0.001f;

// 行高小于 ascender→descender 范围时，必须被抬高到覆盖完整字形。
void test_line_height_covers_glyph_extent()
{
    FontAtlas::Metrics metrics;
    metrics.ascent_ratio = 1.0f;
    metrics.descent_ratio = -0.4f; // FreeType 约定为负
    metrics.line_height_ratio = 1.1f;

    const auto normalized = FontAtlas::normalize_metrics(metrics);
    assert(std::fabs(normalized.line_height_ratio - 1.4f) < kEps);
    // 其他字段不受影响。
    assert(std::fabs(normalized.ascent_ratio - 1.0f) < kEps);
    assert(std::fabs(normalized.descent_ratio + 0.4f) < kEps);
}

// 已经足够大的行高不应被改动。
void test_line_height_kept_when_already_large()
{
    FontAtlas::Metrics metrics;
    metrics.ascent_ratio = 1.0f;
    metrics.descent_ratio = -0.3f;
    metrics.line_height_ratio = 1.6f;

    const auto normalized = FontAtlas::normalize_metrics(metrics);
    assert(std::fabs(normalized.line_height_ratio - 1.6f) < kEps);
}

// 下限：行高至少为 1.0 * size。
void test_line_height_floor()
{
    FontAtlas::Metrics metrics;
    metrics.ascent_ratio = 0.5f;
    metrics.descent_ratio = -0.1f;
    metrics.line_height_ratio = 0.2f;

    const auto normalized = FontAtlas::normalize_metrics(metrics);
    assert(normalized.line_height_ratio >= 1.0f - kEps);
}

// begin_generated 内部也要归一化，measure 的高度应与之吻合。
void test_begin_generated_normalizes()
{
    FontAtlas::Metrics metrics;
    metrics.ascent_ratio = 1.0f;
    metrics.descent_ratio = -0.5f;
    metrics.line_height_ratio = 1.0f;

    FontAtlas atlas;
    atlas.begin_generated(64, metrics, false);

    assert(std::fabs(atlas.metrics().line_height_ratio - 1.5f) < kEps);
    assert(std::fabs(atlas.line_height(20.0f) - 30.0f) < 0.01f);

    // 单行高度 = line_height。
    const auto measured = atlas.measure("A", 20.0f);
    assert(std::fabs(measured.y - 30.0f) < 0.01f);

    // 三行 + 行距。
    const auto three = atlas.measure("A\nA\nA", 20.0f, 4.0f);
    assert(std::fabs(three.y - (30.0f * 3.0f + 8.0f)) < 0.01f);
}
} // namespace

int main()
{
    test_line_height_covers_glyph_extent();
    test_line_height_kept_when_already_large();
    test_line_height_floor();
    test_begin_generated_normalizes();
    std::printf("test_font_atlas_metrics: all passed\n");
    return 0;
}
