// SDF 管线的顶点契约（P1：只扩字段，不改变 kind=0 的行为）。
//
// 这里锁住的是「CPU 顶点布局 == shader attribute location」这条契约：
// 布局写错时 GPU 不会报错，只会画出乱七八糟的几何，所以必须用测试兜住。

#include "graphics/drawable.hpp"
#include "graphics/box.hpp"
#include "graphics/vertex.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{
using xel::graphics::DrawList;
using xel::graphics::Vertex;
using xel::graphics::VertexKind;

// 1) 布局：52 字节、6 个属性、offset 与 offsetof 一致。
void test_layout()
{
    static_assert(sizeof(Vertex) == 52);
    static_assert(offsetof(Vertex, pos) == 0);
    static_assert(offsetof(Vertex, color) == 8);
    static_assert(offsetof(Vertex, uv) == 24);
    static_assert(offsetof(Vertex, half) == 32);
    static_assert(offsetof(Vertex, extra) == 40);
    static_assert(offsetof(Vertex, kind) == 48);

    const auto binding = Vertex::get_binding_description();
    assert(binding.binding == 0);
    assert(binding.stride == sizeof(Vertex));
    assert(binding.inputRate == vk::VertexInputRate::eVertex);

    const auto attributes = Vertex::get_attribute_description();
    assert(attributes.size() == 6);

    // location 必须连续从 0 开始：shader 的 VSInput 字段顺序与之一一对应。
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        assert(attributes[i].location == i);
        assert(attributes[i].binding == 0);
    }
    assert(attributes[0].format == vk::Format::eR32G32Sfloat);
    assert(attributes[1].format == vk::Format::eR32G32B32A32Sfloat);
    assert(attributes[2].format == vk::Format::eR32G32Sfloat);
    assert(attributes[3].format == vk::Format::eR32G32Sfloat);
    assert(attributes[4].format == vk::Format::eR32G32Sfloat);
    assert(attributes[5].format == vk::Format::eR32Uint);

    assert(attributes[0].offset == offsetof(Vertex, pos));
    assert(attributes[1].offset == offsetof(Vertex, color));
    assert(attributes[2].offset == offsetof(Vertex, uv));
    assert(attributes[3].offset == offsetof(Vertex, half));
    assert(attributes[4].offset == offsetof(Vertex, extra));
    assert(attributes[5].offset == offsetof(Vertex, kind));
}

// 2) kind 枚举值必须与 shader 里的常量一致（0/1/2/3）。
void test_kind_values()
{
    assert(static_cast<std::uint32_t>(VertexKind::Sprite) == 0u);
    assert(static_cast<std::uint32_t>(VertexKind::Rect) == 1u);
    assert(static_cast<std::uint32_t>(VertexKind::Ring) == 2u);
    assert(static_cast<std::uint32_t>(VertexKind::Atlas) == 3u);
}

// 3) 默认参数：现有图元（色块/文字/图片）全部落在 kind=0，字段为 0。
void test_default_vertex_is_sprite()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 100.0f, 100.0f});
    auto& batch = list.batch();
    batch.add_vertex({1.0f, 2.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});

    assert(batch.vertices.size() == 1);
    const Vertex& vertex = batch.vertices[0];
    assert(vertex.pos.x == 1.0f && vertex.pos.y == 2.0f);
    assert(vertex.uv.x == 0.5f && vertex.uv.y == 0.5f);
    assert(vertex.kind == 0u);
    assert(vertex.half.x == 0.0f && vertex.half.y == 0.0f);
    assert(vertex.extra.x == 0.0f && vertex.extra.y == 0.0f);
}

// 4) 显式参数：解析形状的打包方式（half / radius / thickness / kind）。
void test_sdf_vertex_packing()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 100.0f, 100.0f});
    auto& batch = list.batch();
    batch.add_vertex({10.0f, 20.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {-30.0f, -10.0f},
                     {40.0f, 20.0f}, {6.0f, 2.0f},
                     static_cast<std::uint32_t>(VertexKind::Ring));

    const Vertex& vertex = batch.vertices[0];
    assert(vertex.uv.x == -30.0f && vertex.uv.y == -10.0f);   // 局部坐标
    assert(vertex.half.x == 40.0f && vertex.half.y == 20.0f); // 外半尺寸
    assert(vertex.extra.x == 6.0f && vertex.extra.y == 2.0f); // radius / thickness
    assert(vertex.kind == 2u);
}

// 5) 透明度乘子对新增字段没有副作用（新增字段必须与颜色无关）。
void test_opacity_does_not_touch_sdf_fields()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 100.0f, 100.0f});
    list.push_opacity(0.5f);
    auto& batch = list.batch();
    batch.add_vertex({0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {8.0f, 4.0f},
                     {3.0f, 0.0f}, static_cast<std::uint32_t>(VertexKind::Rect));

    const Vertex& vertex = batch.vertices[0];
    assert(vertex.color.r == 0.5f && vertex.color.a == 0.5f);
    assert(vertex.half.x == 8.0f && vertex.half.y == 4.0f);
    assert(vertex.extra.x == 3.0f);
    assert(vertex.kind == 1u);
}

// 6) 单个 batch 内可以混装不同 kind：kind 是逐顶点的，不切 batch。
void test_mixed_kinds_share_one_batch()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 100.0f, 100.0f});
    auto& batch = list.batch();
    batch.add_vertex({0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    batch.add_vertex({1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {10.0f, 10.0f},
                     {4.0f, 0.0f}, static_cast<std::uint32_t>(VertexKind::Rect));
    assert(list.batches().size() == 1);
    assert(batch.vertices[0].kind == 0u);
    assert(batch.vertices[1].kind == 1u);
}
// 7) 解析 SDF 圆角矩形：1 quad，uv 是相对中心的局部坐标。
void test_sdf_rect_submission()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    const glm::vec4 rect{10.0f, 20.0f, 120.0f, 60.0f};
    xel::graphics::append_rounded_rect(list, rect, 8.0f, {1.0f, 1.0f, 1.0f, 1.0f});

    std::size_t vertices = 0;
    std::size_t indices = 0;
    for (const auto& batch : list.batches()) {
        vertices += batch.vertices.size();
        indices += batch.indices.size();
        for (const auto& vertex : batch.vertices) {
            assert(vertex.kind == static_cast<std::uint32_t>(VertexKind::Rect));
            // half = 外半尺寸；radius = 8。
            assert(vertex.half.x == 60.0f && vertex.half.y == 30.0f);
            assert(vertex.extra.x == 8.0f && vertex.extra.y == 0.0f);
            // uv 是局部坐标：以矩形中心为原点，顶点正好在 ±half。
            assert(std::fabs(std::fabs(vertex.uv.x) - vertex.half.x) < 1e-4f);
            assert(std::fabs(std::fabs(vertex.uv.y) - vertex.half.y) < 1e-4f);
            // 屏幕坐标 = 中心 + 局部坐标。
            assert(std::fabs(vertex.pos.x - (70.0f + vertex.uv.x)) < 1e-4f);
            assert(std::fabs(vertex.pos.y - (50.0f + vertex.uv.y)) < 1e-4f);
        }
    }
    assert(vertices == 4);
    assert(indices == 6);

    // 半径 clamp 到短边一半。
    DrawList clamped;
    clamped.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    xel::graphics::append_rounded_rect(clamped, rect, 1000.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(clamped.batches()[0].vertices[0].extra.x == 30.0f);

    // 零尺寸/全透明不产出顶点。
    DrawList empty;
    empty.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    xel::graphics::append_rounded_rect(empty, {0.0f, 0.0f, 0.0f, 10.0f}, 4.0f,
                                       {1.0f, 1.0f, 1.0f, 1.0f});
    xel::graphics::append_rounded_rect(empty, rect, 4.0f, {1.0f, 1.0f, 1.0f, 0.0f});
    assert(empty.batches().empty());
}

// 8) 解析圆角环：1 quad，thickness 进 extra.y。
void test_sdf_ring_submission()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    xel::graphics::append_rounded_ring(list, {0.0f, 0.0f, 100.0f, 40.0f}, 6.0f, 2.0f,
                                       {1.0f, 1.0f, 1.0f, 1.0f});
    const auto& batch = list.batches()[0];
    assert(batch.vertices.size() == 4);
    for (const auto& vertex : batch.vertices) {
        assert(vertex.kind == static_cast<std::uint32_t>(VertexKind::Ring));
        assert(vertex.half.x == 50.0f && vertex.half.y == 20.0f);
        assert(vertex.extra.x == 6.0f && vertex.extra.y == 2.0f);
    }

    // 厚度 0 / 透明不产出几何。
    DrawList empty;
    empty.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    xel::graphics::append_rounded_ring(empty, {0.0f, 0.0f, 100.0f, 40.0f}, 6.0f, 0.0f,
                                       {1.0f, 1.0f, 1.0f, 1.0f});
    assert(empty.batches().empty());
}

// 9) 顶点数与缩放/DPI 无关：SDF 只提交一个 quad，AA 在 fragment 里。
void test_sdf_vertex_count_is_scale_independent()
{
    const glm::vec4 rect{0.0f, 0.0f, 200.0f, 60.0f};
    const float radii[] = {2.0f, 12.0f, 30.0f, 1000.0f};
    for (const float radius : radii) {
        DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        xel::graphics::append_rounded_rect(list, rect, radius, {1.0f, 1.0f, 1.0f, 1.0f});
        std::size_t vertices = 0;
        for (const auto& batch : list.batches()) vertices += batch.vertices.size();
        assert(vertices == 4);
    }
}

// 10) 内阴影：四层 SDF 叠叠，每层 1 个 quad。
void test_inset_shadow_uses_sdf()
{
    DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    xel::graphics::append_inset_shadow(list, {0.0f, 0.0f, 200.0f, 40.0f}, 8.0f, 12.0f,
                                       {0.0f, 0.0f, 0.0f, 0.3f});
    std::size_t vertices = 0;
    for (const auto& batch : list.batches()) {
        vertices += batch.vertices.size();
        for (const auto& vertex : batch.vertices) {
            assert(vertex.kind == static_cast<std::uint32_t>(VertexKind::Rect));
        }
    }
    assert(vertices == 16); // 4 层 × 4 顶点
}
} // namespace

int main()
{
    test_layout();
    test_kind_values();
    test_default_vertex_is_sprite();
    test_sdf_vertex_packing();
    test_opacity_does_not_touch_sdf_fields();
    test_mixed_kinds_share_one_batch();
    test_sdf_rect_submission();
    test_sdf_ring_submission();
    test_sdf_vertex_count_is_scale_independent();
    test_inset_shadow_uses_sdf();
    std::cout << "graphics sdf vertex tests passed\n";
    return 0;
}
