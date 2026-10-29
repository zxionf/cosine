#pragma once

#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"
#include "graphics/box.hpp"
#include "graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <string>

namespace xel::ui
{
// 预览区底部的时间标尺（M6.4b）：刻度 + 读秒 + 播放头。
// 语义与时间轴标尺一致（点击 / 拖动即 seek），但这里只表达总进度，
// 单位是「占整个 duration 的比例」，因此不需要像素每秒，窗口缩放自适应。
class PreviewRuler : public Widget
{
public:
    void set_duration(float seconds)
    {
        duration_ = std::max(0.0f, seconds);
        mark_layout_dirty();
    }
    float duration() const { return duration_; }

    void set_time(float seconds) { time_ = std::max(0.0f, seconds); }
    float time() const { return time_; }

    void set_on_seek(std::function<void(float)> handler) { on_seek_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        const float width =
            std::isfinite(constraints.x) && constraints.x > 0.0f ? constraints.x : 320.0f;
        return clamp_size({width, kHeight});
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        scrubbing_ = true;
        seek_to(pos.x);
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!scrubbing_) return false;
        seek_to(pos.x);
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        if (!scrubbing_) return false;
        scrubbing_ = false;
        return true;
    }

    float x_at_time(float seconds) const
    {
        const float usable = std::max(1.0f, global_.size.x - kPad * 2.0f);
        if (duration_ <= 0.0f) return global_.pos.x + kPad;
        const float ratio = std::clamp(seconds / duration_, 0.0f, 1.0f);
        return global_.pos.x + kPad + ratio * usable;
    }

    float time_at_x(float x) const
    {
        if (duration_ <= 0.0f) return 0.0f;
        const float usable = std::max(1.0f, global_.size.x - kPad * 2.0f);
        const float ratio = (x - global_.pos.x - kPad) / usable;
        return std::clamp(ratio, 0.0f, 1.0f) * duration_;
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle track = t.style(StyleRole::Track, ControlState::Normal);
        graphics::append_rounded_rect(list, bounds, corner_radius(track.radius), track.fill);
        if (track.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, corner_radius(track.radius),
                                          track.border_width, track.border);
        }

        list.push_clip(bounds);

        auto* atlas = font_atlas();
        const std::string label = std::format("{:.2f} / {:.2f}s", time_, duration_);
        const float label_w =
            atlas != nullptr ? atlas->measure_width(label, t.small_text_size) : 0.0f;
        const float label_left = global_.pos.x + global_.size.x - kPad - label_w;

        const float step = tick_step();
        for (float seconds = 0.0f; seconds <= duration_ + 1.0e-4f; seconds += step) {
            const float x = x_at_time(seconds);
            graphics::Rectangle{x, global_.pos.y + global_.size.y - kTickHeight, 1.0f, kTickHeight,
                                track.border}
                .append_to(list);

            // 与右侧时间文本重叠的刻度文字不画（刻度线照旧），否则叠成一团。
            if (atlas == nullptr || x + 4.0f + label_w > label_left - 8.0f) continue;
            atlas->append_text(list, x + 4.0f, global_.pos.y + 3.0f, t.small_text_size,
                               std::format("{:.1f}s", seconds), t.text);
        }

        // 播放头。
        const float head = x_at_time(time_);
        graphics::Rectangle{head - 1.0f, global_.pos.y, 2.0f, global_.size.y, t.accent}
            .append_to(list);

        // 当前时间 / 总时长：右侧带底色的标签，遵住下方刻度线不被文字穿插。
        if (atlas != nullptr) {
            const glm::vec4 pill{label_left - 8.0f, global_.pos.y + 1.0f, label_w + 14.0f,
                                 global_.size.y - 2.0f};
            const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
            graphics::append_rounded_rect(list, pill, corner_radius(surface.radius), surface.fill);
            atlas->append_text(list, label_left, global_.pos.y + 3.0f, t.small_text_size, label,
                               t.text);
        }

        list.pop_clip();
    }

private:
    static constexpr float kHeight = 28.0f;
    static constexpr float kPad = 10.0f;
    static constexpr float kTickHeight = 6.0f;

    void seek_to(float x)
    {
        const float seconds = time_at_x(x);
        if (on_seek_) on_seek_(seconds);
    }

    // 相邻刻度至少 72px（要给 “0.2s” 这种文字留出呼吸空间）。
    float tick_step() const
    {
        constexpr float kCandidates[] = {0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f, 60.0f};
        const float usable = std::max(1.0f, global_.size.x - kPad * 2.0f);
        if (duration_ <= 0.0f) return 1.0f;
        for (float step : kCandidates) {
            if (step / duration_ * usable >= 72.0f) return step;
        }
        return duration_;
    }

    float duration_ = 5.0f;
    float time_ = 0.0f;
    bool scrubbing_ = false;
    std::function<void(float)> on_seek_;
};
} // namespace xel::ui
