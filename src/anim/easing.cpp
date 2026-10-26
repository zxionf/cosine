#include "easing.hpp"

#include <algorithm>
#include <cmath>

namespace xel::anim
{
namespace
{
// 单轴三次贝塞尔：p0 = 0、p3 = 1。
float bezier_axis(float s, float p1, float p2)
{
    const float u = 1.0f - s;
    return 3.0f * u * u * s * p1 + 3.0f * u * s * s * p2 + s * s * s;
}

float bezier_x(float s, float x1, float x2) { return bezier_axis(s, x1, x2); }
float bezier_y(float s, float y1, float y2) { return bezier_axis(s, y1, y2); }
} // namespace

float CubicBezier::operator()(float x) const
{
    if (!(x > 0.0f)) return 0.0f; // 同时挡住 NaN
    if (x >= 1.0f) return 1.0f;

    // clamp 控制点保证 x(s) 单调，逆函数唯一。
    const float px1 = std::clamp(x1, 0.0f, 1.0f);
    const float px2 = std::clamp(x2, 0.0f, 1.0f);

    // 二分求 s：x(s) = x。24 次迭代精度约 6e-8，足够且不会发散。
    float lo = 0.0f;
    float hi = 1.0f;
    for (int i = 0; i < 24; ++i) {
        const float mid = 0.5f * (lo + hi);
        if (bezier_x(mid, px1, px2) < x) lo = mid;
        else hi = mid;
    }
    const float s = 0.5f * (lo + hi);
    return bezier_y(s, y1, y2);
}

float ease(Easing easing, float t)
{
    const float x = std::clamp(t, 0.0f, 1.0f);
    switch (easing) {
        case Easing::Linear: return x;
        case Easing::EaseIn: return kEaseIn(x);
        case Easing::EaseOut: return kEaseOut(x);
        case Easing::EaseInOut: return kEaseInOut(x);
        case Easing::CubicBezier: return kEaseInOut(x);
    }
    return x;
}

float ease(Easing easing, float t, const CubicBezier& custom)
{
    if (easing == Easing::CubicBezier) return custom(std::clamp(t, 0.0f, 1.0f));
    return ease(easing, t);
}
} // namespace xel::anim
