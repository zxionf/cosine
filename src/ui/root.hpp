#pragma once

#include "ui/widget.hpp"
#include "ui/uicontext.hpp"

#include <functional>
#include <memory>

namespace xel::ui
{
// UIRoot 是自由画布：子控件保留自己的局部坐标，Root 只负责 global bounds 与裁剪边界。
// overlays 位于普通子项之上的独立图层，先命中、后绘制，用于 Tooltip/弹窗等。
class UIRoot : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::Custom; }
    UIRoot(float x, float y, float width, float height);
    ~UIRoot() = default;

    void add(std::unique_ptr<Widget> child) override;
    // 延迟移除普通子项（与 remove_overlay 同语义）：事件派发期间仍可能持有指针，
    // 实际析构发生在下一次 layout() 的 purge_trash()。
    void remove(Widget* child);
    // overlay 按加入顺序叠放，后加入的在上层；尺寸由 measure 决定。
    void add_overlay(std::unique_ptr<Widget> child);
    // 延迟移除：事件派发过程中可能仍持有目标指针，实际销毁发生在下一次 layout()。
    void remove_overlay(Widget* overlay);
    void clear_overlays() { overlays_.clear(); mark_layout_dirty(); }

    // 模态栈：窗口进入模态时撑一个全屏 Scrim（z = 窗口 z - 1），支持嵌套。
    // with_scrim=false 用于自带遮罩的模态宿主（如 DialogHost）。
    void push_modal(Widget* window, bool with_scrim = true);
    void pop_modal(Widget* window);
    Widget* modal_top() const { return modal_frames_.empty() ? nullptr : modal_frames_.back().window; }
    bool is_modal(Widget* window) const;

    // 焦点钩子：由 UISystem 注入 getter/setter（UIRoot 不依赖 UISystem，避免循环）。
    using FocusGetter = std::function<Widget*()>;
    using FocusSetter = std::function<void(Widget*)>;
    void set_focus_hooks(FocusGetter getter, FocusSetter setter);
    void clear_focus_hooks();

    // 切换主题：更新 context 并递归刷新控件缓存的样式值（如 Panel 的 padding/spacing）。
    void set_theme(const Theme& value);
    const Theme& ui_theme() const { return context_storage_.theme(); }

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
    // 模态帧：window / scrim / prev_focus 三份状态合并为一个元素，避免三数组错位。
    struct ModalFrame
    {
        Widget* window = nullptr;
        Widget* scrim = nullptr;      // scrim 同时也在 children_ 里（参与命中与绘制）
        Widget* prev_focus = nullptr; // 打开该层前的焦点快照（pop 时恢复）
    };
    std::vector<ModalFrame> modal_frames_;
    FocusGetter focus_getter_;
    FocusSetter focus_setter_;
    std::vector<std::unique_ptr<Widget>> trash_;
};
} // namespace xel::ui
