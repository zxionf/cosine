#pragma once

#include "widget.hpp"
#include "align.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace xel::ui
{
// 流式换行：子项按顺序从左到右排，放不下就换行。
// 用于工具栏、标签云、卡片流这类「数量不定、宽度不一」的内容。
class Wrap : public Widget
{
public:
    Wrap(float x, float y, float width, float height);
    Wrap(glm::vec2 pos, glm::vec2 size);
    ~Wrap() override;

    void add(std::unique_ptr<Widget> child) override;
    void clear_children() { children_.clear(); mark_layout_dirty(); }
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    void append_to(graphics::DrawList& list) const override;

    void layout() override;
    bool hit_testable() const override { return false; }
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;
    void invalidate_measure() override { measure_valid_ = false; }

    void set_spacing(float all) { set_spacing(all, all); }
    void set_spacing(float dx, float dy)
    {
        spacing_x_ = dx;
        spacing_y_ = dy;
        mark_layout_dirty();
    }
    float spacing_x() const { return spacing_x_; }
    float spacing_y() const { return spacing_y_; }

    // 行内剩余宽的分配方式：Start 靠左（默认）/ Center / End / SpaceBetween。
    void set_row_align(MainAlign a)
    {
        row_align_ = a;
        mark_layout_dirty();
    }
    MainAlign row_align() const { return row_align_; }

    // 行内子项的垂直对齐；子项可用 set_align() 单独覆盖。
    void set_cross_align(Align a)
    {
        cross_align_ = a;
        mark_layout_dirty();
    }
    Align cross_align() const { return cross_align_; }

    Align resolve_align(const Widget& child) const;

private:
    struct Row
    {
        std::vector<std::size_t> items; // 流式子项在 children_ 中的索引
        float width = 0.0f;             // 含 margin 与行内间距
        float height = 0.0f;            // 含 margin
    };

    // 按可用宽折行；unbounded 为真时全部排成一行。
    std::vector<Row> layout_rows(float inner_w, bool unbounded);

    float spacing_x_ = 4.0f;
    float spacing_y_ = 4.0f;
    MainAlign row_align_ = MainAlign::Start;
    Align cross_align_ = Align::center_left();

    mutable glm::vec2 cached_measure_{0.0f, 0.0f};
    mutable glm::vec2 cached_constraints_{-1.0f, -1.0f};
    mutable bool measure_valid_ = false;

};
} // namespace xel::ui
