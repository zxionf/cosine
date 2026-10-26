#pragma once

namespace xel::anim
{
// 缓动曲线：把归一化时间 t ∈ [0,1] 映射到进度 [0,1]。
enum class Easing
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    CubicBezier, // 使用传入的自定义控制点
};

// 三次贝塞尔：控制点 (0,0)、(x1,y1)、(x2,y2)、(1,1)。
// x1 / x2 在求值时被 clamp 到 [0,1]，保证 x(s) 单调、可以求逆。
struct CubicBezier
{
    float x1 = 0.25f;
    float y1 = 0.10f;
    float x2 = 0.25f;
    float y2 = 1.00f;

    // 给定时间 x ∈ [0,1]，返回进度 y。
    float operator()(float x) const;
};

// 预置曲线（CSS 标准控制点）。
inline constexpr CubicBezier kEaseIn{0.42f, 0.0f, 1.0f, 1.0f};
inline constexpr CubicBezier kEaseOut{0.0f, 0.0f, 0.58f, 1.0f};
inline constexpr CubicBezier kEaseInOut{0.42f, 0.0f, 0.58f, 1.0f};

// t 会先 clamp 到 [0,1]；CubicBezier 模式使用默认控制点。
float ease(Easing easing, float t);

// CubicBezier 模式使用 custom；其它枚举忽略 custom。
float ease(Easing easing, float t, const CubicBezier& custom);
} // namespace xel::anim
