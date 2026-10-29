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
    // 延迟移除：事件派发过程中可能仍持有目标指针，实际销毁发生在下一次 layout()。
    void remove_overlay(Widget* overlay);
    void clear_overlays() { overlays_.clear(); mark_layout_dirty(); }

    // 模态栈：窗口进入模态时撑一个全屏 Scrim（z = 窗口 z - 1），支持嵌套。
    void push_modal(Widget* window);
    void pop_modal(Widget* window);
    Widget* modal_top() const { return modals_.empty() ? nullptr : modals_.back(); }
    bool is_modal(Widget* window) const;

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
    void purge_trash();

    UIContext context_storage_;
    std::vector<std::unique_ptr<Widget>> overlays_;
    // 与 modals_ 一一对应；scrim 同时也在 children_ 里（参与命中与绘制）。
    std::vector<Widget*> modals_;
    std::vector<Widget*> modal_scrims_;
    std::vector<std::unique_ptr<Widget>> trash_;
};
} // namespace xel::ui
