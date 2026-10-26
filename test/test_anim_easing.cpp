#include "anim/easing.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
bool nearly(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) < eps;
}

const xel::anim::Easing kModes[] = {
    xel::anim::Easing::Linear,
    xel::anim::Easing::EaseIn,
    xel::anim::Easing::EaseOut,
    xel::anim::Easing::EaseInOut,
    xel::anim::Easing::CubicBezier,
};

void test_endpoints_and_clamp()
{
    for (auto mode : kModes) {
        assert(nearly(xel::anim::ease(mode, 0.0f), 0.0f, 1e-6f));
        assert(nearly(xel::anim::ease(mode, 1.0f), 1.0f, 1e-6f));
        // 越界输入被 clamp，不会外插。
        assert(nearly(xel::anim::ease(mode, -2.0f), 0.0f, 1e-6f));
        assert(nearly(xel::anim::ease(mode, 3.0f), 1.0f, 1e-6f));
    }
}

void test_linear_identity()
{
    for (int i = 0; i <= 10; ++i) {
        const float t = static_cast<float>(i) / 10.0f;
        assert(nearly(xel::anim::ease(xel::anim::Easing::Linear, t), t, 1e-6f));
    }
}

void test_monotonic_and_bounded()
{
    for (auto mode : kModes) {
        float prev = -1.0f;
        for (int i = 0; i <= 100; ++i) {
            const float t = static_cast<float>(i) / 100.0f;
            const float v = xel::anim::ease(mode, t);
            assert(v >= prev - 1e-5f);
            assert(v >= -1e-5f && v <= 1.0f + 1e-5f);
            prev = v;
        }
    }
}

void test_ease_out_ahead_of_ease_in()
{
    const float t = 0.5f;
    assert(xel::anim::ease(xel::anim::Easing::EaseOut, t)
           > xel::anim::ease(xel::anim::Easing::EaseIn, t));
}

void test_custom_bezier()
{
    const xel::anim::CubicBezier custom{0.1f, 0.9f, 0.2f, 1.0f};
    assert(nearly(custom(0.0f), 0.0f, 1e-6f));
    assert(nearly(custom(1.0f), 1.0f, 1e-6f));

    // 控制点 y 偏高 → 中点进度大于 0.5。
    const float mid = custom(0.5f);
    assert(mid > 0.5f);
    assert(nearly(xel::anim::ease(xel::anim::Easing::CubicBezier, 0.5f, custom), mid));

    // 非 CubicBezier 模式忽略 custom。
    assert(nearly(xel::anim::ease(xel::anim::Easing::Linear, 0.25f, custom), 0.25f, 1e-6f));
}

void test_extreme_controls()
{
    const xel::anim::CubicBezier left{0.0f, 0.0f, 0.0f, 1.0f};
    const xel::anim::CubicBezier right{1.0f, 0.0f, 1.0f, 1.0f};
    for (int i = 0; i <= 10; ++i) {
        const float t = static_cast<float>(i) / 10.0f;
        const float a = left(t);
        const float b = right(t);
        assert(!std::isnan(a) && !std::isnan(b));
        assert(a >= -1e-5f && a <= 1.0f + 1e-5f);
        assert(b >= -1e-5f && b <= 1.0f + 1e-5f);
    }
}

void test_presets_differ()
{
    const float t = 0.35f;
    const float in = xel::anim::ease(xel::anim::Easing::EaseIn, t);
    const float out = xel::anim::ease(xel::anim::Easing::EaseOut, t);
    const float inout = xel::anim::ease(xel::anim::Easing::EaseInOut, t);
    assert(!nearly(in, out, 1e-3f));
    assert(inout > in && inout < out);
}
} // namespace

int main()
{
    test_endpoints_and_clamp();
    test_linear_identity();
    test_monotonic_and_bounded();
    test_ease_out_ahead_of_ease_in();
    test_custom_bezier();
    test_extreme_controls();
    test_presets_differ();
    std::printf("test_anim_easing: all passed\n");
    return 0;
}
