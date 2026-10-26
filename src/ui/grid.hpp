#pragma once

#include "widget.hpp"
#include "align.hpp"

#include <memory>
#include <vector>

namespace xel::ui
{
// 多列网格：列宽可等分，也可「固定像素 + 弹性权重」混合；行高按内容自动取最大。
// 用于表单（标签列固定、控件列填充）与规则宫格，避免嵌套 Hbox 后列宽无法对齐。
class Grid : public Widget
{
public:
    Grid(float x, float y, float width, float height);
    Grid(glm::vec2 pos, glm::vec2 size);
    ~Grid() override;

    void add(std::unique_ptr<Widget> child) override;
    void clear_children() { children_.clear(); mark_layout_dirty(); }
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    void append_to(graphics::DrawList& list) const override;

    void layout() override;
    bool hit_testable() const override { return false; }
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;
    void invalidate_measure() override { measure_valid_ = false; }

    // 等分列；等价于 set_column_widths(count 个 -1)。count <= 0 回退 1 列。
    void set_columns(int count);
    // > 0 固定像素；<= 0 弹性列，按 |值| 分享剩余宽度（-1 即常见的 1fr）。空列表回退 1 列。
    void set_column_widths(std::vector<float> widths);
    std::vector<float> column_widths() const { return widths_; }
    int columns() const { return static_cast<int>(widths_.empty() ? 1 : widths_.size()); }

    void set_spacing(float all) { set_spacing(all, all); }
    void set_spacing(float dx, float dy)
    {
        spacing_x_ = dx;
        spacing_y_ = dy;
        mark_layout_dirty();
    }
    float spacing_x() const { return spacing_x_; }
    float spacing_y() const { return spacing_y_; }

    // 单元格内对齐；子项可用 set_align() 单独覆盖。默认横向填满、纵向靠上。
    void set_cell_align(Align a)
    {
        cell_align_ = a;
        mark_layout_dirty();
    }
    Align cell_align() const { return cell_align_; }

private:
    struct ColumnLayout
    {
        std::vector<float> offset; // 每列左边界（相对内容区原点）
        std::vector<float> width;
    };

    // 按可用宽度解析列宽：固定列先占位，弹性列分享剩余；固定列超宽时等比压缩。
    ColumnLayout resolve_columns(float inner_w) const;
    Align resolve_cell_align(const Widget& child) const;

    std::vector<float> widths_{-1.0f};
    float spacing_x_ = 4.0f;
    float spacing_y_ = 4.0f;
    Align cell_align_ = Align::h_stretch();

    mutable glm::vec2 cached_measure_{0.0f, 0.0f};
    mutable glm::vec2 cached_constraints_{-1.0f, -1.0f};
    mutable bool measure_valid_ = false;

    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui
