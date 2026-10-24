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

    float scroll_offset() const { return offset_; }

    glm::vec2 measure(glm::vec2 constraints) override;
    void      arrange(Rect slot) override;

    bool on_scroll(glm::vec2 pos, float delta) override;
    bool hit_testable() const override { return false; }
    bool clips_children() const override { return true; }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

    void append_to(graphics::DrawBatch& batch) const override;

private:
    // children_ 持有内容，content_ 是非拥有别名，供布局与绘制使用。
    std::vector<std::unique_ptr<Widget>> children_;
    Widget*   content_ = nullptr;
    glm::vec2 content_size_{};
    float     offset_ = 0.f;
    Axis      axis_ = Axis::Vertical;
};
} // namespace xel::ui
