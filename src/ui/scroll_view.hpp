#pragma once

#include "widget.hpp"

namespace xel::ui
{
class ScrollView : public Widget {
public:
    enum class Axis { Vertical, Horizontal, Both };

    ScrollView() = default;
    ~ScrollView() = default;

    void set_axis(Axis a)          { axis_ = a; /*mark_layout_dirty();*/ }
    void set_content(std::unique_ptr<Widget> content);

    glm::vec2 content_size() const { return content_size_; }
    glm::vec2 viewport_size() const { return global_.size; }

    float scroll_offset() const { return offset_; }
    // void  set_scroll_offset(float o);

    glm::vec2 measure(glm::vec2 constraints) override;
    void      arrange(Rect slot) override;

    bool on_scroll(glm::vec2 pos, float delta) override;   // ★ 事件入口
    bool hit_testable() const override { return false; }   // 容器，不吃命中

    void append_to(graphics::DrawBatch& batch) const override;

private:
    std::unique_ptr<Widget> content_;
    glm::vec2 content_size_{};
    float     offset_ = 0.f;          // 只支持单轴时用 float
    glm::vec2 offset2_{};             // 双轴时用 vec2
    Axis      axis_ = Axis::Vertical;
};
} // namespace xel::ui