#include "graphics/font_atlas.hpp"
#include "graphics/text_debug.hpp"
#include "ui/label.hpp"
#include "ui/typography.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
using xel::graphics::DrawList;
using xel::graphics::FontAtlas;
using xel::graphics::TextDebugOptions;
using xel::graphics::TextureSlot;

constexpr float kEps = 0.01f;

struct Bounds
{
    float min_x = 1.0e9f;
    float min_y = 1.0e9f;
    float max_x = -1.0e9f;
    float max_y = -1.0e9f;

    bool empty() const { return min_x > max_x; }
};

Bounds vertex_bounds(const DrawList& list, TextureSlot slot, bool any_slot = false)
{
    Bounds bounds;
    for (const auto& batch : list.batches()) {
        if (!any_slot && batch.texture_slot != slot) continue;
        for (const auto& v : batch.vertices) {
            bounds.min_x = std::min(bounds.min_x, v.pos.x);
            bounds.min_y = std::min(bounds.min_y, v.pos.y);
            bounds.max_x = std::max(bounds.max_x, v.pos.x);
            bounds.max_y = std::max(bounds.max_y, v.pos.y);
        }
    }
    return bounds;
}

// 合成图集：一个 8x10 的 'A'，advance 0.6，bearing (0.1, 0.7)，基准字号 10。
FontAtlas make_atlas()
{
    FontAtlas atlas;
    std::vector<unsigned char> pixels(64 * 64 * 4, 0);
    atlas.load(64, 64, pixels.data());

    std::vector<unsigned char> bitmap(8 * 10, 200);
    const bool added = atlas.add_glyph(U'A', bitmap.data(), 8, 10, 8, 0.6f, 0.1f, 0.7f, 10.0f);
    assert(added);
    return atlas;
}

float glyph_x(const DrawList& list)
{
    return vertex_bounds(list, TextureSlot::Texture).min_x;
}

void test_debug_boxes_match_glyph_quad()
{
    FontAtlas atlas = make_atlas();
    const float size = 10.0f;

    DrawList text_list;
    atlas.append_text(text_list, 10.0f, 20.0f, size, "A", {1.0f, 1.0f, 1.0f, 1.0f});
    const Bounds glyph = vertex_bounds(text_list, TextureSlot::Texture);
    assert(!glyph.empty());

    TextDebugOptions options;
    options.glyph_boxes = true;

    DrawList debug_list;
    xel::graphics::append_text_debug(debug_list, atlas, 10.0f, 20.0f, size, "A", options);
    const Bounds box = vertex_bounds(debug_list, TextureSlot::White, true);
    assert(!box.empty());

    // 包围盒应与实际字形四边形完全重合，否则位移/度量不一致。
    assert(std::fabs(box.min_x - glyph.min_x) < kEps);
    assert(std::fabs(box.min_y - glyph.min_y) < kEps);
    assert(std::fabs(box.max_x - glyph.max_x) < kEps);
    assert(std::fabs(box.max_y - glyph.max_y) < kEps);

    // 字形几何：x0 = 10 + 0.1*10，基线 = 20 + ascent*10。
    assert(std::fabs(glyph.min_x - 11.0f) < kEps);
    const float baseline = 20.0f + atlas.metrics().ascent_ratio * size;
    // 合成字形：上沿在基线上方 0.7em，高度 1em，因此下沿 = baseline + 0.3em。
    assert(std::fabs(glyph.max_y - (baseline + 3.0f)) < kEps);
}

void test_baseline_and_line_box()
{
    FontAtlas atlas = make_atlas();
    const float size = 10.0f;
    const float line_h = atlas.line_height(size);

    TextDebugOptions options;
    options.line_box = true;
    options.wrap_width = 1000.0f; // 不换行

    DrawList list;
    xel::graphics::append_text_debug(list, atlas, 0.0f, 0.0f, size, "AA", options);
    const Bounds box = vertex_bounds(list, TextureSlot::White, true);
    assert(std::fabs(box.min_y - 0.0f) < kEps);
    assert(std::fabs(box.max_y - line_h) < kEps);

    // 两行 + 4px 行距：第二行行框应整体下移 line_h + 4。
    options.baseline = true;
    DrawList two_lines;
    xel::graphics::append_text_debug(two_lines, atlas, 0.0f, 0.0f, size, "A\nA", options);
    const Bounds both = vertex_bounds(two_lines, TextureSlot::White, true);
    assert(std::fabs(both.min_y - 0.0f) < kEps);
    // 基线用 1px 粗线居中绘制，允许半像素外扩。
    assert(std::fabs(both.max_y - (line_h * 2.0f)) <= 0.75f);
}

void test_debug_wrap_matches_measured_lines()
{
    FontAtlas atlas = make_atlas();
    const float size = 10.0f;
    const float advance = 6.0f;

    // 每行只放得下 2 个字：宽度 = 2*advance。
    const float wrap_width = advance * 2.0f + 0.5f;
    const auto measured = atlas.measure_wrapped("AAAAA", size, wrap_width);
    assert(std::fabs(measured.x - advance * 2.0f) < kEps);
    assert(std::fabs(measured.y - atlas.line_height(size) * 3.0f) < kEps);

    TextDebugOptions options;
    options.line_box = true;
    options.wrap_width = wrap_width;

    DrawList list;
    xel::graphics::append_text_debug(list, atlas, 0.0f, 0.0f, size, "AAAAA", options);
    const Bounds box = vertex_bounds(list, TextureSlot::White, true);
    // 调试层也必须折成 3 行，否则行框会和文本错位。
    assert(std::fabs(box.max_y - atlas.line_height(size) * 3.0f) < kEps);
    // 字形墨迹可能超出 advance，这里只校验行数（高度）与折行位置一致。
    assert(box.max_x < wrap_width + advance);
}

void test_line_spacing_measure()
{
    FontAtlas atlas = make_atlas();
    const float size = 10.0f;
    const float line_h = atlas.line_height(size);

    const auto tight = atlas.measure("A\nA\nA", size);
    const auto spaced = atlas.measure("A\nA\nA", size, 5.0f);

    assert(std::fabs(tight.y - line_h * 3.0f) < kEps);
    assert(std::fabs(spaced.y - (line_h * 3.0f + 10.0f)) < kEps);

    // 换行测量使用同样的行距语义。
    const auto wrapped = atlas.measure_wrapped("AA\nAA", size, 1000.0f, 5.0f);
    assert(std::fabs(wrapped.y - (line_h * 2.0f + 5.0f)) < kEps);
}

void test_label_alignment_and_snap()
{
    FontAtlas atlas = make_atlas();
    xel::graphics::set_active_font_atlas(&atlas);

    const float advance = 6.0f; // 单字 'A' 的宽度

    xel::ui::Label left("A");
    left.set_text_size(10.0f);
    left.arrange({{100.0f, 10.0f}, {200.0f, 16.0f}});
    DrawList left_list;
    left.append_to(left_list);
    assert(std::fabs(glyph_x(left_list) - 101.0f) < kEps); // 101 = 100 + 0.1*10

    xel::ui::Label center("A");
    center.set_text_size(10.0f);
    center.set_text_align(xel::ui::TextAlign::Center);
    center.arrange({{100.0f, 10.0f}, {200.0f, 16.0f}});
    DrawList center_list;
    center.append_to(center_list);
    const float expected_center = 100.0f + (200.0f - advance) * 0.5f + 1.0f;
    assert(std::fabs(glyph_x(center_list) - expected_center) < kEps);

    xel::ui::Label right("A");
    right.set_text_size(10.0f);
    right.set_text_align(xel::ui::TextAlign::Right);
    right.arrange({{100.0f, 10.0f}, {200.0f, 16.0f}});
    DrawList right_list;
    right.append_to(right_list);
    const float expected_right = 100.0f + 200.0f - advance + 1.0f;
    assert(std::fabs(glyph_x(right_list) - expected_right) < kEps);

    // 像素对齐：0.4 的亚像素偏移被抹平。
    xel::ui::Label snapped("A");
    snapped.set_text_size(10.0f);
    snapped.arrange({{100.4f, 10.4f}, {200.0f, 16.0f}});
    DrawList snapped_list;
    snapped.append_to(snapped_list);
    const float snapped_x = glyph_x(snapped_list);
    assert(std::fabs(snapped_x - std::round(snapped_x)) < kEps);

    xel::ui::Label unsnapped("A");
    unsnapped.set_text_size(10.0f);
    unsnapped.set_pixel_snap(false);
    unsnapped.arrange({{100.4f, 10.4f}, {200.0f, 16.0f}});
    DrawList unsnapped_list;
    unsnapped.append_to(unsnapped_list);
    const float raw_x = glyph_x(unsnapped_list);
    assert(std::fabs(raw_x - (101.4f)) < kEps);
    assert(std::fabs(raw_x - snapped_x) > 0.2f);

    xel::graphics::set_active_font_atlas(nullptr);
}

void test_text_role_sizes()
{
    xel::ui::Theme theme;
    theme.text_size = 16.0f;

    const auto body = xel::ui::text_style(xel::ui::TextRole::Body, theme);
    const auto title = xel::ui::text_style(xel::ui::TextRole::Title, theme);
    const auto caption = xel::ui::text_style(xel::ui::TextRole::Caption, theme);

    assert(std::fabs(body.size - 16.0f) < kEps);
    assert(title.size > body.size);
    assert(caption.size < body.size);
    assert(caption.color != theme.text);

    xel::ui::Heading heading("Hi", 2);
    assert(heading.text_size() == xel::ui::text_style(xel::ui::TextRole::Heading, theme).size);
    heading.set_level(1);
    assert(heading.text_size() > xel::ui::text_style(xel::ui::TextRole::Heading, theme).size);

    xel::ui::Paragraph paragraph("text");
    assert(paragraph.wrap());
}
} // namespace

int main()
{
    test_debug_boxes_match_glyph_quad();
    test_baseline_and_line_box();
    test_debug_wrap_matches_measured_lines();
    test_line_spacing_measure();
    test_label_alignment_and_snap();
    test_text_role_sizes();

    std::cout << "text display tests passed\n";
    return 0;
}
