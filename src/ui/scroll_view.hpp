#pragma once

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
    glm::vec2 viewport_size() const { return global_.size; }

    float scroll_offset()   const { return offset_.y; }
    float scroll_offset_x() const { return offset_.x; }
    glm::vec2 scroll_offset_vec() const { return offset_; }
    void set_scroll_offset(glm::vec2 offset);

    glm::vec2 measure(glm::vec2 constraints) override;
    void      arrange(Rect slot) override;

    bool on_scroll(glm::vec2 pos, glm::vec2 delta) override;
    bool hit_testable() const override { return false; }
    bool clips_children() const override { return true; }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

    void append_to(graphics::DrawList& list) const override;

private:
    glm::vec2 content_constraints(glm::vec2 constraints) const;
    glm::vec2 clamp_offset(glm::vec2 offset, glm::vec2 viewport) const;

    // children_ 持有内容，content_ 是非拥有别名，供布局与绘制使用。
    std::vector<std::unique_ptr<Widget>> children_;
    Widget*   content_ = nullptr;
    glm::vec2 content_size_{};
    glm::vec2 offset_{0.0f, 0.0f};
    Axis      axis_ = Axis::Vertical;
};
} // namespace xel::ui
