// 圆角的顶点级抗锯齿（fringe）契约测试。
//
// 目标是锁住「不改 shader 也能 AA」的几何结构：
//   内圈顶点 = 原色（预乘），外圈顶点 = 全 0（透明），两者相距 1 物理像素；
//   轮廓一遍生成，圆弧段数按屏幕半径与弦高误差动态定。

#include "graphics/box.hpp"
#include "graphics/drawable.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
using xel::graphics::DrawList;
using xel::graphics::TextureSlot;

struct Stats
{
    std::size_t vertices = 0;
    std::size_t indices = 0;
    std::size_t transparent_vertices = 0;
};

Stats collect(const DrawList& list)
{
    Stats stats;
    for (const auto& batch : list.batches()) {
        stats.vertices += batch.vertices.size();
        stats.indices += batch.indices.size();
        for (const auto& vertex : batch.vertices) {
            if (vertex.color.a <= 0.0f) ++stats.transparent_vertices;
        }
        for (const uint32_t index : batch.indices) assert(index < batch.vertices.size());
    }
    return stats;
}

DrawList make_list(float pixel_ratio = 1.0f)
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    list.set_pixel_ratio(pixel_ratio);
    return list;
}

bool near(float a, float b, float epsilon = 1e-3f) { return std::fabs(a - b) <= epsilon; }

// 1) 段数：随屏幕半径增长、受上限约束、DPI 参与计算。
void test_arc_segments()
{
    using xel::graphics::box_detail::arc_segments;

    const int small = arc_segments(4.0f);
    const int medium = arc_segments(8.0f);
    const int large = arc_segments(100.0f);
    assert(small >= 2);
    assert(medium > small);
    assert(large > medium);
    assert(large <= xel::graphics::box_detail::kMaxArcSegments);
    assert(arc_segments(0.1f) == 2);

    // 弦高误差的上界：每段角度对应的 sagitta 不超过 0.3 物理像素。
    const float radius = 50.0f;
    const int segments = arc_segments(radius);
    const float step = (xel::graphics::box_detail::kPi * 0.5f) / static_cast<float>(segments);
    const float sagitta = radius * (1.0f - std::cos(step));
    assert(sagitta <= xel::graphics::box_detail::kMaxArcErrorPx + 1e-3f);
}

// 2) 填充：内圈不透明、外圈全 0、顶点数 = 8*seg + 1（中心）。
void test_fill_structure()
{
    const glm::vec4 rect{10.0f, 20.0f, 100.0f, 40.0f};
    const float radius = 6.0f;
    const glm::vec4 color{1.0f, 0.5f, 0.25f, 0.8f};

    DrawList list = make_list();
    xel::graphics::append_rounded_rect(list, rect, radius, color);

    const int segments = xel::graphics::box_detail::arc_segments(
        radius * list.pixel_ratio());
    const Stats stats = collect(list);
    assert(stats.vertices == static_cast<std::size_t>(8 * segments + 1));
    assert(stats.indices % 3 == 0);
    // 外圈 4*seg 个顶点全透明，内圈与中心不透明。
    assert(stats.transparent_vertices == static_cast<std::size_t>(4 * segments));

    // 预乘：内圈 rgb = color.rgb * a，外圈必须全 0（否则会加亮）。
    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) {
            if (vertex.color.a <= 0.0f) {
                assert(vertex.color.r == 0.0f && vertex.color.g == 0.0f &&
                       vertex.color.b == 0.0f && vertex.color.a == 0.0f);
            } else {
                assert(near(vertex.color.a, color.a));
                assert(near(vertex.color.r, color.r * color.a));
                assert(near(vertex.color.g, color.g * color.a));
                assert(near(vertex.color.b, color.b * color.a));
            }
        }
    }
}

// 3) fringe 宽度 = 1 物理像素：pixel_ratio=2 时逻辑距离减半。
void test_fringe_width_follows_pixel_ratio()
{
    const glm::vec4 rect{0.0f, 0.0f, 200.0f, 100.0f};
    for (const float ratio : {1.0f, 2.0f, 1.5f}) {
        DrawList list = make_list(ratio);
        xel::graphics::append_rounded_rect(list, rect, 12.0f, {1.0f, 1.0f, 1.0f, 1.0f});

        // 找上边中点附近的一对（内圈, 外圈）顶点：x 相同、y 小的那个是外圈。
        float best = 1e9f;
        for (const auto& batch : list.batches()) {
            for (const auto& inner : batch.vertices) {
                if (inner.color.a <= 0.0f) continue;
                for (const auto& outer : batch.vertices) {
                    if (outer.color.a > 0.0f) continue;
                    if (!near(inner.pos.x, outer.pos.x, 0.5f)) continue;
                    if (!near(inner.pos.y, 0.0f, 0.01f)) continue; // 只在顶边
                    best = std::min(best, std::fabs(outer.pos.y - inner.pos.y));
                }
            }
        }
        assert(best < 1e8f);
        assert(near(best, 1.0f / ratio, 0.05f));
    }
}

// 4) 外扩量：直边处 1px，接合点最多 1.42px（1/cos45°）。
void test_fringe_bounds()
{
    const glm::vec4 rect{0.0f, 0.0f, 200.0f, 100.0f};
    DrawList list = make_list();
    xel::graphics::append_rounded_rect(list, rect, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f});

    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) {
            // 所有顶点（含外圈）不超出矩形 1.5px。
            assert(vertex.pos.x >= rect.x - 1.5f);
            assert(vertex.pos.y >= rect.y - 1.5f);
            assert(vertex.pos.x <= rect.x + rect.z + 1.5f);
            assert(vertex.pos.y <= rect.y + rect.w + 1.5f);
        }
    }
}

// 5) 段数随 pixel_ratio 增长：同一逻辑半径在高 DPI 下需要更多段。
void test_segments_follow_pixel_ratio()
{
    const glm::vec4 rect{0.0f, 0.0f, 200.0f, 100.0f};
    DrawList low = make_list(1.0f);
    xel::graphics::append_rounded_rect(low, rect, 20.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    DrawList high = make_list(3.0f);
    xel::graphics::append_rounded_rect(high, rect, 20.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(high).vertices > collect(low).vertices);
}

// 6) 环：条带 + 内外 fringe；skip_outer_fringe 只砍掉外侧那圈。
void test_ring_fringe()
{
    const glm::vec4 rect{0.0f, 0.0f, 120.0f, 60.0f};
    const float radius = 8.0f;
    const int segments = xel::graphics::box_detail::arc_segments(radius);
    const int count = 4 * segments;

    DrawList full = make_list();
    xel::graphics::append_rounded_ring(full, rect, radius, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    // 外轮廓 + 内轮廓 + 外 fringe + 内 fringe。
    assert(collect(full).vertices == static_cast<std::size_t>(4 * count));
    assert(collect(full).transparent_vertices == static_cast<std::size_t>(2 * count));

    DrawList skipped = make_list();
    skipped.set_skip_outer_fringe(true);
    xel::graphics::append_rounded_ring(skipped, rect, radius, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(skipped).vertices == static_cast<std::size_t>(3 * count));
    assert(collect(skipped).transparent_vertices == static_cast<std::size_t>(count));
}

// 7) 关掉 fringe（内阴影多层叠加用）时不产生透明顶点。
void test_fill_without_fringe()
{
    const glm::vec4 rect{0.0f, 0.0f, 120.0f, 40.0f};
    DrawList list = make_list();
    xel::graphics::append_rounded_rect(list, rect, 8.0f, {1.0f, 1.0f, 1.0f, 1.0f}, false);
    const int segments = xel::graphics::box_detail::arc_segments(8.0f);
    assert(collect(list).vertices == static_cast<std::size_t>(4 * segments + 1));
    assert(collect(list).transparent_vertices == 0);

    DrawList shadow = make_list();
    xel::graphics::append_inset_shadow(shadow, rect, 8.0f, 10.0f, {0.0f, 0.0f, 0.0f, 0.4f});
    assert(collect(shadow).transparent_vertices == 0);
}

// 8) 退化与边界：小半径走矩形、超大半径 clamp、零尺寸/全透明不产出顶点。
void test_degenerate_cases()
{
    const glm::vec4 rect{0.0f, 0.0f, 100.0f, 40.0f};

    DrawList tiny = make_list();
    xel::graphics::append_rounded_rect(tiny, rect, 0.5f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(tiny).vertices == 4);

    DrawList clamped = make_list();
    xel::graphics::append_rounded_rect(clamped, rect, 1000.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    DrawList exact = make_list();
    xel::graphics::append_rounded_rect(exact, rect, 20.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(clamped).vertices == collect(exact).vertices);

    DrawList empty = make_list();
    xel::graphics::append_rounded_rect(empty, {0.0f, 0.0f, 0.0f, 40.0f}, 6.0f,
                                       {1.0f, 1.0f, 1.0f, 1.0f});
    xel::graphics::append_rounded_rect(empty, rect, 6.0f, {1.0f, 1.0f, 1.0f, 0.0f});
    xel::graphics::append_rounded_ring(empty, rect, 8.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(empty).vertices == 0);
}

// 9) 旧路径仍可用，且与新的契约（索引合法、不超界）一致。
void test_hard_path_still_available()
{
    const glm::vec4 rect{0.0f, 0.0f, 100.0f, 40.0f};
    DrawList hard = make_list();
    xel::graphics::append_rounded_rect_hard(hard, rect, 8.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    DrawList soft = make_list();
    xel::graphics::append_rounded_rect(soft, rect, 8.0f, {1.0f, 1.0f, 1.0f, 1.0f});

    // 旧路径没有透明顶点（硬边），新路径有。
    assert(collect(hard).transparent_vertices == 0);
    assert(collect(soft).transparent_vertices > 0);
    assert(collect(hard).vertices > 4);

    DrawList hard_ring = make_list();
    xel::graphics::append_rounded_ring_hard(hard_ring, rect, 8.0f, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(collect(hard_ring).vertices > 16);
    assert(collect(hard_ring).transparent_vertices == 0);
}

// 10) 批量合并：同槽同 clip 仍是一个 batch，fringe 不额外切批。
void test_no_extra_batches()
{
    DrawList list = make_list();
    for (int i = 0; i < 20; ++i) {
        xel::graphics::append_rounded_rect(
            list, {static_cast<float>(i) * 12.0f, 0.0f, 10.0f, 10.0f}, 4.0f,
            {1.0f, 1.0f, 1.0f, 1.0f});
    }
    assert(list.batches().size() == 1);
    assert(list.batches()[0].texture_slot == static_cast<uint32_t>(TextureSlot::Texture));
}
} // namespace

int main()
{
    test_arc_segments();
    test_fill_structure();
    test_fringe_width_follows_pixel_ratio();
    test_fringe_bounds();
    test_segments_follow_pixel_ratio();
    test_ring_fringe();
    test_fill_without_fringe();
    test_degenerate_cases();
    test_hard_path_still_available();
    test_no_extra_batches();
    std::cout << "graphics fringe tests passed\n";
    return 0;
}
