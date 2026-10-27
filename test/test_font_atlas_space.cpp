#include "graphics/font_atlas.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

namespace
{
using namespace xel;

std::vector<unsigned char> make_bitmap(int size)
{
    return std::vector<unsigned char>(static_cast<std::size_t>(size) * size, 255);
}

void test_used_ratio_grows()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(64, metrics, false);

    assert(atlas.used_ratio() >= 0.0f && atlas.used_ratio() <= 1.0f);

    std::vector<unsigned char> bitmap = make_bitmap(8);
    assert(atlas.add_glyph(U'A', bitmap.data(), 8, 8, 8, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(atlas.used_ratio() > 0.0f);
}

void test_out_of_space_is_retryable()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(32, metrics, false);

    int calls = 0;
    atlas.set_glyph_loader([&calls](graphics::FontAtlas& target, char32_t codepoint, int pixel_size) {
        ++calls;
        // 24×24：32×32 图集每行只能放一个。
        std::vector<unsigned char> bitmap = make_bitmap(24);
        return target.add_glyph(codepoint, bitmap.data(), 24, 24, 24, 0.5f, 0.0f, 0.5f,
                                static_cast<float>(pixel_size));
    });

    // 第一个字形应成功。
    assert(atlas.ensure_glyph(U'A', 16));

    // 第二个必然空间不足。
    const int before = calls;
    const bool second = atlas.ensure_glyph(U'B', 16);
    assert(!second);
    assert(atlas.last_add_out_of_space());
    assert(!atlas.has_glyph(U'B', 16));

    // 关键：空间不足不能被永久记入 missing——再次请求应重新尝试 loader。
    atlas.ensure_glyph(U'B', 16);
    assert(calls >= before + 2);
}

void test_missing_glyph_is_cached()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(64, metrics, false);

    int calls = 0;
    atlas.set_glyph_loader([&calls](graphics::FontAtlas&, char32_t, int) {
        ++calls;
        return false; // 模拟「字符不存在」：没有调用 add_glyph
    });

    assert(!atlas.ensure_glyph(U'\u4e00', 16));
    assert(calls == 1);

    // 缺字应被缓存，不重复调用 loader。
    assert(!atlas.ensure_glyph(U'\u4e00', 16));
    assert(calls == 1);
}

void test_dirty_rect_tracks_new_glyphs()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(64, metrics, false);

    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    assert(!atlas.take_dirty_rect(x, y, w, h)); // 初始无脏区

    std::vector<unsigned char> bitmap = make_bitmap(8);
    assert(atlas.add_glyph(U'A', bitmap.data(), 8, 8, 8, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(atlas.take_dirty_rect(x, y, w, h));
    assert(w == 8 && h == 8);

    // 取出后清零。
    assert(!atlas.take_dirty_rect(x, y, w, h));

    // 连续新增多个字形合并成一个包围盒。
    assert(atlas.add_glyph(U'B', bitmap.data(), 8, 8, 8, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(atlas.add_glyph(U'C', bitmap.data(), 8, 8, 8, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(atlas.take_dirty_rect(x, y, w, h));
    assert(w >= 8 && h == 8);
}

void test_empty_glyph_has_no_dirty_rect()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(64, metrics, false);

    // 空白字形没有像素，不应产生脏区。
    atlas.add_empty_glyph(U' ', 0.5f, 16.0f);
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    assert(!atlas.take_dirty_rect(x, y, w, h));
}

void test_add_glyph_resets_space_flag()
{
    graphics::FontAtlas atlas;
    graphics::FontAtlas::Metrics metrics;
    atlas.begin_generated(32, metrics, false);

    std::vector<unsigned char> big = make_bitmap(24);
    assert(atlas.add_glyph(U'A', big.data(), 24, 24, 24, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(!atlas.last_add_out_of_space());

    assert(!atlas.add_glyph(U'B', big.data(), 24, 24, 24, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(atlas.last_add_out_of_space());

    // 后续成功添加（小字形放得下）应重置标志。
    std::vector<unsigned char> tiny = make_bitmap(2);
    assert(atlas.add_glyph(U'C', tiny.data(), 2, 2, 2, 0.5f, 0.0f, 0.5f, 16.0f));
    assert(!atlas.last_add_out_of_space());
}
} // namespace

int main()
{
    test_used_ratio_grows();
    test_out_of_space_is_retryable();
    test_missing_glyph_is_cached();
    test_add_glyph_resets_space_flag();
    test_dirty_rect_tracks_new_glyphs();
    test_empty_glyph_has_no_dirty_rect();
    std::printf("test_font_atlas_space: all passed\n");
    return 0;
}
