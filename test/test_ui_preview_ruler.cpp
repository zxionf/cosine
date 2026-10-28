#include "ui/preview_ruler.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
using xel::ui::PreviewRuler;

constexpr float kEps = 1.0e-3f;

// 时间与像素互转必须能往返，否则拖到哪儿都不会落在预期时间上。
void test_time_x_round_trip()
{
    PreviewRuler ruler;
    ruler.set_duration(4.0f);
    ruler.arrange({{0.0f, 0.0f}, {410.0f, 28.0f}});

    // 左右各留 10px 边距，可用宽 390。
    assert(std::fabs(ruler.x_at_time(0.0f) - 10.0f) < kEps);
    assert(std::fabs(ruler.x_at_time(4.0f) - 400.0f) < kEps);
    assert(std::fabs(ruler.x_at_time(2.0f) - 205.0f) < kEps);

    for (float t : {0.0f, 0.37f, 1.0f, 2.5f, 3.99f, 4.0f}) {
        const float back = ruler.time_at_x(ruler.x_at_time(t));
        if (std::fabs(back - t) > 1.0e-2f) {
            std::fprintf(stderr, "round trip failed at %.3f -> %.3f\n", t, back);
            std::abort();
        }
    }
}

// 点击任意位置 seek 到对应时间，并夹在 [0, duration]。
void test_click_seek()
{
    PreviewRuler ruler;
    ruler.set_duration(2.0f);
    ruler.arrange({{0.0f, 0.0f}, {220.0f, 28.0f}});

    float seeked = -1.0f;
    ruler.set_on_seek([&seeked](float t) { seeked = t; });

    // 可用宽 200（左右各 10），x=110 对应 0.5 → 时间 1.0s；x=115 → 1.05s。
    assert(ruler.on_mouse_down({115.0f, 14.0f}, 0));
    assert(std::fabs(seeked - 1.05f) < 1.0e-2f);
    assert(ruler.on_mouse_up({115.0f, 14.0f}, 0));

    // 拖出左边界 → 夹到 0。
    ruler.on_mouse_down({100.0f, 14.0f}, 0);
    ruler.on_mouse_move({-50.0f, 14.0f});
    assert(seeked == 0.0f);
    // 拖出右边界 → 夹到 duration。
    ruler.on_mouse_move({900.0f, 14.0f});
    assert(std::fabs(seeked - 2.0f) < kEps);
    ruler.on_mouse_up({900.0f, 14.0f}, 0);
}

// 未按下时移动不 seek。
void test_move_without_press_ignored()
{
    PreviewRuler ruler;
    ruler.set_duration(2.0f);
    ruler.arrange({{0.0f, 0.0f}, {210.0f, 28.0f}});

    int calls = 0;
    ruler.set_on_seek([&calls](float) { ++calls; });
    assert(!ruler.on_mouse_move({100.0f, 14.0f}));
    assert(calls == 0);
}

// duration 为 0 时不应除零，且一切映射到左端。
void test_zero_duration()
{
    PreviewRuler ruler;
    ruler.set_duration(0.0f);
    ruler.arrange({{0.0f, 0.0f}, {210.0f, 28.0f}});

    assert(std::fabs(ruler.x_at_time(1.0f) - 10.0f) < kEps);
    assert(ruler.time_at_x(500.0f) == 0.0f);

    float seeked = -1.0f;
    ruler.set_on_seek([&seeked](float t) { seeked = t; });
    assert(ruler.on_mouse_down({150.0f, 14.0f}, 0));
    assert(seeked == 0.0f);
}

// 高度固定 28，宽度跟随约束。
void test_measure()
{
    PreviewRuler ruler;
    ruler.set_duration(2.0f);
    const glm::vec2 size = ruler.measure({500.0f, 120.0f});
    assert(std::fabs(size.x - 500.0f) < kEps);
    assert(std::fabs(size.y - 28.0f) < kEps);
}
} // namespace

int main()
{
    test_time_x_round_trip();
    test_click_seek();
    test_move_without_press_ignored();
    test_zero_duration();
    test_measure();
    std::printf("test_ui_preview_ruler: all passed\n");
    return 0;
}
