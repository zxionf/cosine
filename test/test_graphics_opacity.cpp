#include "graphics/drawable.hpp"
#include "graphics/rectangle.hpp"

#include <cmath>
#include <cstdio>

using namespace xel;

namespace
{
constexpr float kEps = 1e-5f;

bool near(float a, float b)
{
    return std::fabs(a - b) < kEps;
}

void test_default_is_opaque()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 0.0f, 0.0f, 1.0f}}.append_to(list);

    const auto& vertices = list.batches().front().vertices;
    if (!near(vertices[0].color.a, 1.0f)) std::fprintf(stderr, "default alpha != 1\n");
    if (!near(vertices[0].color.r, 1.0f)) std::fprintf(stderr, "default red changed\n");
    std::printf("default opaque ok\n");
}

void test_half_opacity_scales_all_channels()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    list.push_opacity(0.5f);
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 0.0f, 0.0f, 1.0f}}.append_to(list);
    list.pop_opacity();

    const auto& v = list.batches().front().vertices;
    // 预乘 alpha：四个通道同乘 0.5。
    if (!near(v[0].color.r, 0.5f)) std::fprintf(stderr, "r not scaled\n");
    if (!near(v[0].color.a, 0.5f)) std::fprintf(stderr, "a not scaled\n");
    std::printf("half opacity ok\n");
}

void test_nested_opacity_multiplies()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    list.push_opacity(0.5f);
    list.push_opacity(0.5f);
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f}}.append_to(list);

    const auto& v = list.batches().front().vertices;
    if (!near(v[0].color.a, 0.25f)) std::fprintf(stderr, "nested alpha = %f\n", v[0].color.a);
    std::printf("nested opacity ok\n");
}

void test_pop_restores_previous()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    list.push_opacity(0.5f);
    list.pop_opacity();
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f}}.append_to(list);

    const auto& v = list.batches().front().vertices;
    if (!near(v[0].color.a, 1.0f)) std::fprintf(stderr, "pop did not restore\n");
    std::printf("pop restores ok\n");
}

void test_clear_resets_stack()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    list.push_opacity(0.1f);
    list.clear();
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f}}.append_to(list);

    const auto& v = list.batches().front().vertices;
    if (!near(v[0].color.a, 1.0f)) std::fprintf(stderr, "clear did not reset opacity\n");
    if (!near(list.opacity(), 1.0f)) std::fprintf(stderr, "opacity() not reset\n");
    std::printf("clear resets ok\n");
}

void test_clamped_to_unit_range()
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 640.0f, 480.0f});
    list.push_opacity(2.0f); // 超过 1 应被夹住，不会让颜色变亮
    graphics::Rectangle{0.0f, 0.0f, 10.0f, 10.0f, {0.5f, 0.5f, 0.5f, 1.0f}}.append_to(list);
    list.pop_opacity();

    const auto& v = list.batches().front().vertices;
    if (!near(v[0].color.r, 0.5f)) std::fprintf(stderr, "opacity > 1 brightened color\n");
    std::printf("clamp ok\n");
}
} // namespace

int main()
{
    test_default_is_opaque();
    test_half_opacity_scales_all_channels();
    test_nested_opacity_multiplies();
    test_pop_restores_previous();
    test_clear_resets_stack();
    test_clamped_to_unit_range();
    std::printf("graphics opacity tests passed\n");
    return 0;
}
