#include "graphics/font_atlas.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

namespace
{
using xel::graphics::FontAtlas;

// pixel_ratio 让字形按「逻辑字号 × 比例」栅格化，绘制尺寸仍是逻辑字号。
void test_pixel_ratio_scales_glyph_request()
{
    FontAtlas atlas;
    FontAtlas::Metrics metrics;
    atlas.begin_generated(256, metrics, false);
    assert(atlas.pixel_ratio() == 1.0f);

    int requested = 0;
    atlas.set_glyph_loader([&requested](FontAtlas& target, char32_t codepoint, int pixel_size) {
        requested = pixel_size;
        std::vector<unsigned char> bitmap(2 * 2, 255);
        return target.add_glyph(codepoint, bitmap.data(), 2, 2, 2, 0.5f, 0.0f, 0.5f,
                                static_cast<float>(pixel_size));
    });

    // 比例 1：请求逻辑字号本身。
    atlas.measure("A", 10.0f);
    assert(requested == 10);

    // 比例 2：请求 20 号物理字形。
    atlas.set_pixel_ratio(2.0f);
    atlas.measure("B", 10.0f);
    assert(requested == 20);

    // 比例 1.5：10 * 1.5 = 15。
    atlas.set_pixel_ratio(1.5f);
    atlas.measure("C", 10.0f);
    assert(requested == 15);
}

void test_pixel_ratio_guard()
{
    FontAtlas atlas;
    atlas.set_pixel_ratio(0.0f);   // 非法值回落 1.0
    assert(atlas.pixel_ratio() == 1.0f);

    atlas.set_pixel_ratio(-2.0f);
    assert(atlas.pixel_ratio() == 1.0f);

    atlas.set_pixel_ratio(2.0f);
    assert(atlas.pixel_ratio() == 2.0f);

    // 量化仍遵守 [4, 256]。
    assert(atlas.glyph_pixel_size(1.0f) == 4);
    assert(atlas.glyph_pixel_size(200.0f) == 256);
}

// 非整数比例下，物理字号量化到最近整数，且与绘制尺寸无关。
void test_glyph_pixel_size_quantizes()
{
    FontAtlas atlas;
    atlas.set_pixel_ratio(1.25f);
    // 13 * 1.25 = 16.25 → 16
    assert(atlas.glyph_pixel_size(13.0f) == 16);
    // 14 * 1.25 = 17.5 → 18
    assert(atlas.glyph_pixel_size(14.0f) == 18);
}
} // namespace

int main()
{
    test_pixel_ratio_scales_glyph_request();
    test_pixel_ratio_guard();
    test_glyph_pixel_size_quantizes();
    std::printf("test_font_atlas_dpi: all passed\n");
    return 0;
}
