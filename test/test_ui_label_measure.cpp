#include "graphics/font_atlas.hpp"
#include "ui/label.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using xel::graphics::DrawList;
using xel::graphics::FontAtlas;
using xel::graphics::TextureSlot;

constexpr float kEps = 0.01f;

FontAtlas make_atlas(float advance_ratio)
{
    FontAtlas atlas;
    std::vector<unsigned char> pixels(64 * 64 * 4, 0);
    atlas.load(64, 64, pixels.data());

    std::vector<unsigned char> bitmap(8 * 10, 200);
    const bool added = atlas.add_glyph(U'A', bitmap.data(), 8, 10, 8, advance_ratio, 0.1f, 0.7f, 10.0f);
    assert(added);
    // 同字形再补一档 20 号，用于验证「字号变化使缓存失效」。
    const bool added20 = atlas.add_glyph(U'A', bitmap.data(), 8, 10, 8, advance_ratio, 0.1f, 0.7f, 20.0f);
    assert(added20);
    return atlas;
}

float glyph_x(const DrawList& list)
{
    float min_x = 1.0e9f;
    for (const auto& batch : list.batches()) {
        if (batch.texture_slot != TextureSlot::Texture) continue;
        for (const auto& v : batch.vertices) min_x = std::min(min_x, v.pos.x);
    }
    return min_x;
}

// 切换图集后缓存必须失效：同一个 Label 的测量结果要跟着图集走。
void test_cache_invalidated_on_atlas_switch()
{
    FontAtlas atlas_a = make_atlas(0.6f);
    FontAtlas atlas_b = make_atlas(0.9f);

    xel::graphics::set_active_font_atlas(&atlas_a);
    xel::ui::Label label("AA");
    label.set_text_size(10.0f);

    const auto a = label.measure({1000.0f, 1000.0f});
    assert(std::fabs(a.x - 12.0f) < kEps);

    // 换图集：advance 0.9 → 宽 18。
    xel::graphics::set_active_font_atlas(&atlas_b);
    const auto b = label.measure({1000.0f, 1000.0f});
    assert(std::fabs(b.x - 18.0f) < kEps);

    // 切回：应重新计算回 12，而不是复用 18。
    xel::graphics::set_active_font_atlas(&atlas_a);
    const auto again = label.measure({1000.0f, 1000.0f});
    assert(std::fabs(again.x - 12.0f) < kEps);

    xel::graphics::set_active_font_atlas(nullptr);
}

void test_cache_invalidated_on_text_and_size()
{
    FontAtlas atlas = make_atlas(0.6f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("AA");
    label.set_text_size(10.0f);
    assert(std::fabs(label.measure({1000.0f, 1000.0f}).x - 12.0f) < kEps);

    label.set_text("AAA");
    assert(std::fabs(label.measure({1000.0f, 1000.0f}).x - 18.0f) < kEps);

    label.set_text_size(20.0f);
    assert(std::fabs(label.measure({1000.0f, 1000.0f}).x - 36.0f) < kEps);

    xel::graphics::set_active_font_atlas(nullptr);
}

void test_cache_invalidated_on_line_spacing()
{
    FontAtlas atlas = make_atlas(0.6f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("A\nA");
    label.set_text_size(10.0f);
    const auto tight = label.measure({1000.0f, 1000.0f});

    label.set_line_spacing(5.0f);
    const auto spaced = label.measure({1000.0f, 1000.0f});
    assert(spaced.y > tight.y + 1.0f);

    xel::graphics::set_active_font_atlas(nullptr);
}

void test_cache_invalidated_on_wrap_and_max_lines()
{
    FontAtlas atlas = make_atlas(0.6f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("AAAA");
    label.set_text_size(10.0f);
    label.set_wrap(true);

    // 宽度 10、advance 6：每行一个字。
    label.set_max_lines(1);
    const auto one = label.measure({10.0f, 1000.0f});

    label.set_max_lines(2);
    const auto two = label.measure({10.0f, 1000.0f});
    assert(two.y > one.y + 1.0f);

    // 关闭换行后回到单行测量。
    label.set_wrap(false);
    const auto flat = label.measure({10.0f, 1000.0f});
    assert(std::fabs(flat.x - 24.0f) < kEps);

    xel::graphics::set_active_font_atlas(nullptr);
}

// 左对齐不测量：绘制原点就是控件左边（加 bearing），与既有行为一致。
void test_left_align_skips_measure()
{
    FontAtlas atlas = make_atlas(0.6f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("A");
    label.set_text_size(10.0f);
    label.arrange({{100.0f, 10.0f}, {200.0f, 16.0f}});

    DrawList list;
    label.append_to(list);
    // bearing_x_ratio = 0.1 → 100 + 1
    assert(std::fabs(glyph_x(list) - 101.0f) < kEps);

    xel::graphics::set_active_font_atlas(nullptr);
}

// 居中仍按文本宽度偏移，且与 measure 结果一致。
void test_center_uses_measured_width()
{
    FontAtlas atlas = make_atlas(0.6f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("A");
    label.set_text_size(10.0f);
    label.set_text_align(xel::ui::TextAlign::Center);
    label.arrange({{100.0f, 10.0f}, {200.0f, 16.0f}});

    DrawList list;
    label.append_to(list);
    // 宽 6、可用 200 → 偏移 97；加 bearing 1 → 198
    assert(std::fabs(glyph_x(list) - 198.0f) < kEps);

    xel::graphics::set_active_font_atlas(nullptr);
}
} // namespace

int main()
{
    test_cache_invalidated_on_atlas_switch();
    test_cache_invalidated_on_text_and_size();
    test_cache_invalidated_on_line_spacing();
    test_cache_invalidated_on_wrap_and_max_lines();
    test_left_align_skips_measure();
    test_center_uses_measured_width();
    std::printf("test_ui_label_measure: all passed\n");
    return 0;
}
