#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "scroll_mode.hpp"

#include <vector>

namespace xel::ui
{
class ScrollView : public Widget {
public:
    enum class Axis { Vertical, Horizontal, Both };

    ScrollView() = default;
    ~ScrollView() override = default;

    void set_axis(Axis a)          { axis_ = a; mark_layout_dirty(); }
    void set_content(std::unique_ptr<Widget> content);

    glm::vec2 content_size() const { return content_size_; }
    // 可滚动区域（已扣除可见滚动条占用的空间）。
    glm::vec2 viewport_size() const { return viewport_; }

    float scroll_offset()   const { return offset_.y; }
    float scroll_offset_x() const { return offset_.x; }
    glm::vec2 scroll_offset_vec() const { return offset_; }
    void set_scroll_offset(glm::vec2 offset);

    // 滚动模式：Smooth = 滚轮只改目标、实际位置平滑逼近；Line = 立即到位。
    void set_scroll_mode(ScrollMode mode) { scroll_mode_ = mode; }
    ScrollMode scroll_mode() const { return scroll_mode_; }
    // 平滑时间常数（秒）；0 = 立即到位。
    void set_scroll_smoothing(float seconds) { smoothing_ = seconds > 0.0f ? seconds : 0.0f; }
    float scroll_smoothing() const { return smoothing_; }
    // Smooth 模式每格滚轮的像素步长。
    void set_scroll_speed(float pixels_per_step) { scroll_speed_ = pixels_per_step; }
    float scroll_speed() const { return scroll_speed_; }
    // Line 模式每格滚轮的像素步长。
    void set_scroll_line_step(float pixels_per_line) { line_step_ = pixels_per_line; }
    float scroll_line_step() const { return line_step_; }
    // 目标偏移（Smooth 模式下 offset 会向它收敛）。
    glm::vec2 scroll_target() const { return target_; }

    // 滚动条可见性与几何，供测试与命中使用。
    bool scrollbar_visible(bool vertical) const;
    Rect scrollbar_track_rect(bool vertical) const;
    Rect scrollbar_thumb_rect(bool vertical) const;

    glm::vec2 measure(glm::vec2 constraints) override;
    void      arrange(Rect slot) override;

    bool on_scroll(glm::vec2 pos, glm::vec2 delta) override;
    bool on_mouse_down(glm::vec2 pos, int button) override;
    bool on_mouse_move(glm::vec2 pos) override;
    bool on_mouse_up(glm::vec2 pos, int button) override;
    void on_update(float dt) override;

    bool hit_testable() const override { return true; }
    bool clips_children() const override { return true; }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

    void append_to(graphics::DrawList& list) const override;

private:
    enum class Drag { None, Vertical, Horizontal };

    glm::vec2 content_constraints(glm::vec2 constraints) const;
    glm::vec2 clamp_offset(glm::vec2 offset) const;
    float max_scroll(bool vertical) const;
    float thumb_length(bool vertical, const Rect& track) const;
    // 计算 content_size_ / viewport_ / 滚动条可见性。
    void update_metrics(glm::vec2 outer);
    void place_content();

    // children_ 持有内容，content_ 是非拥有别名，供布局与绘制使用。
    Widget*   content_ = nullptr;
    glm::vec2 content_size_{};
    glm::vec2 viewport_{};
    glm::vec2 offset_{0.0f, 0.0f};
    glm::vec2 target_{0.0f, 0.0f};
    ScrollMode scroll_mode_ = ScrollMode::Smooth;
    float smoothing_ = 0.12f;
    float scroll_speed_ = 40.0f;
    float line_step_ = 40.0f;
    // 最近一次滚轮输入后的空闲时长，供 Fast 模式判断「该收尾了」。
    float idle_ = 0.0f;
    Axis      axis_ = Axis::Vertical;
    bool      show_v_ = false;
    bool      show_h_ = false;

    Drag  dragging_ = Drag::None;
    float drag_start_offset_ = 0.0f;
    float drag_start_pos_ = 0.0f;
};
} // namespace xel::ui
