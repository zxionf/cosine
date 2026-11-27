#pragma once

#include "ui/geom/layout_metrics.hpp"
#include "ui/node/widget.hpp"

#include <memory>

namespace xel::ui
{
// 单轴流式布局内核（UI-36）：Vbox 与 Hbox 曾各写一份 measure/arrange/flex/margin，
// 行为已经漂移（Hbox 缺 flex 主轴特例等）。现在两者都退化成「选一个轴」的薄封装：
//
//   Axis::Vertical   → Vbox（主轴 y，交叉轴 x）
//   Axis::Horizontal → Hbox（主轴 x，交叉轴 y）
//
// 尺寸解算交给 ui/layout_budget.hpp 的 resolve_main_axis（pct 预算 + flex grow/shrink），
// 这里只负责：内容区计算、主轴对齐（Start/Center/End/SpaceBetween）、交叉轴对齐与 stretch、
// 绝对定位子项摆放。
class LinearLayout : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::Panel; }
    enum class Axis
    {
        Vertical,
        Horizontal,
    };

    void add(std::unique_ptr<Widget> child) override;
    // clear_children() 由 Widget 提供（UI-49：清空 + 标脏），派生类不再重写。
    void append_to(graphics::DrawList& list) const override;

    void layout() override;
    bool hit_testable() const override { return false; }
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;

    Axis axis() const { return axis_; }
    bool horizontal() const { return axis_ == Axis::Horizontal; }

    // 间距：显式调用即锁定，主题重刷不再覆盖（UI-35 语义）。
    void set_spacing(float spacing)
    {
        spacing_ = spacing;
        spacing_override_ = true;
        mark_layout_dirty();
    }
    void apply_theme_spacing(float spacing);
    bool spacing_override() const { return spacing_override_; }
    void clear_spacing_override() { spacing_override_ = false; }
    float spacing() const { return spacing_; }

    void set_cross_align(Align a)
    {
        cross_align_ = a;
        mark_layout_dirty();
    }
    Align cross_align() const { return cross_align_; }
    void set_main_align(MainAlign a)
    {
        main_align_ = a;
        mark_layout_dirty();
    }
    MainAlign main_align() const { return main_align_; }
    Align resolve_align(const Widget& child) const;

    // 裸容器跟随主题度量（UI-35）：显式设过的值优先。
    void set_context(UIContext* context) override;

protected:
    explicit LinearLayout(Axis axis);

    // 子类构造期用：设置结构性默认值（不锁定主题）。
    void set_axis_defaults(Align cross_align);

private:
    Axis axis_ = Axis::Vertical;
    float spacing_ = 4.0f;
    bool spacing_override_ = false;
    Align cross_align_ = Align::top_center();
    MainAlign main_align_ = MainAlign::Start;
};
} // namespace xel::ui
