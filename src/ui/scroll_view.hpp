#pragma once

#include "theme.hpp"
#include "widget.hpp"

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
    std::vector<std::unique_ptr<Widget>> children_;
    Widget*   content_ = nullptr;
    glm::vec2 content_size_{};
    glm::vec2 viewport_{};
    glm::vec2 offset_{0.0f, 0.0f};
    Axis      axis_ = Axis::Vertical;
    bool      show_v_ = false;
    bool      show_h_ = false;

    Drag  dragging_ = Drag::None;
    float drag_start_offset_ = 0.0f;
    float drag_start_pos_ = 0.0f;
};
} // namespace xel::ui
