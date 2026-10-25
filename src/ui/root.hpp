#pragma once

#include "widget.hpp"

#include <memory>

namespace xel::ui
{
// UIRoot 是自由画布：子控件保留自己的局部坐标，Root 只负责 global bounds 与裁剪边界。
// overlays 位于普通子项之上的独立图层，先命中、后绘制，用于 Tooltip/弹窗等。
class UIRoot : public Widget
{
public:
    UIRoot(float x, float y, float width, float height);
    ~UIRoot() = default;

    void add(std::unique_ptr<Widget> child) override;
    // overlay 按加入顺序叠放，后加入的在上层；尺寸由 measure 决定。
    void add_overlay(std::unique_ptr<Widget> child);
    void clear_overlays() { overlays_.clear(); mark_layout_dirty(); }

    // 切换主题：更新 context 并递归刷新控件缓存的样式值（如 Panel 的 padding/spacing）。
    void set_theme(const Theme& value);
    const Theme& ui_theme() const { return context_storage_.theme; }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    const std::vector<std::unique_ptr<Widget>>& overlays() const override { return overlays_; }
    bool hit_testable() const override { return false; }
    void append_to(graphics::DrawList& list) const override;
    void layout() override;
    glm::vec2 measure(glm::vec2 constraints) override;
    void arrange(Rect slot) override;

private:
    void layout_child(Widget& child);

    UIContext context_storage_;
    std::vector<std::unique_ptr<Widget>> children_;
    std::vector<std::unique_ptr<Widget>> overlays_;
};
} // namespace xel::ui
