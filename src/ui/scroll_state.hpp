#pragma once

#include "scroll_mode.hpp"

#include <algorithm>
#include <cmath>

namespace xel::ui
{
// 滚动状态：值 / 目标 / 可视与内容尺寸 / 平滑逼近。
// 目的：把 Table、ScrollView、TextInput 各自实现的 clamp + 平滑收敛到一处。
class ScrollState
{
public:
    void set_mode(ScrollMode mode) { mode_ = mode; }
    ScrollMode mode() const { return mode_; }

    void set_smoothing(float seconds) { smoothing_ = std::max(0.0f, seconds); }
    float smoothing() const { return smoothing_; }

    // 视口/内容尺寸变了就重新夹取，避免窗口缩小后停在越界位置。
    void set_range(float viewport, float content)
    {
        viewport_ = std::max(0.0f, viewport);
        content_ = std::max(0.0f, content);
        offset_ = clamp(offset_);
        target_ = clamp(target_);
    }
    float viewport() const { return viewport_; }
    float content() const { return content_; }
    float max_offset() const { return std::max(0.0f, content_ - viewport_); }
    bool scrollable() const { return max_offset() > 0.0f; }

    float offset() const { return offset_; }
    float target() const { return target_; }

    // 立即到位（键盘、程序化设置、结构变化）。
    void jump_to(float value)
    {
        offset_ = clamp(value);
        target_ = offset_;
    }

    // 滚轮/拖拽：immediate=true 时直接到位，否则只改目标由 update() 逼近。
    void scroll_by(float delta, bool immediate)
    {
        if (immediate) {
            jump_to(offset_ + delta);
        } else {
            target_ = clamp(target_ + delta);
        }
    }

    // 平滑逼近目标（与 Table 的手感一致：指数衰减 + Fast 模式加速 + snap 阀值）。
    // 返回是否仍在动画中。
    bool update(float dt)
    {
        if (mode_ == ScrollMode::Line || dt <= 0.0f) return false;
        if (!scrollable()) {
            offset_ = 0.0f;
            target_ = 0.0f;
            return false;
        }
        const float diff = target_ - offset_;
        if (std::fabs(diff) < 0.01f) {
            idle_ = 0.0f;
            return false;
        }
        idle_ += dt;

        float tau = smoothing_;
        float snap = 0.25f;
        if (mode_ == ScrollMode::Fast) {
            // 基础更快；停止输入 0.1s 后再加速收尾。
            tau = std::max(0.03f, smoothing_ * 0.4f);
            snap = 0.5f;
            if (idle_ > 0.1f) tau *= 0.5f;
        }

        const float k = tau > 0.0f ? (1.0f - std::exp(-dt / tau)) : 1.0f;
        offset_ = clamp(offset_ + diff * k);
        if (std::fabs(target_ - offset_) < snap) offset_ = target_;
        return true;
    }

private:
    float clamp(float value) const { return std::clamp(value, 0.0f, max_offset()); }

    ScrollMode mode_ = ScrollMode::Smooth;
    float smoothing_ = 0.12f;
    float viewport_ = 0.0f;
    float content_ = 0.0f;
    float offset_ = 0.0f;
    float target_ = 0.0f;
    float idle_ = 0.0f;
};
} // namespace xel::ui
