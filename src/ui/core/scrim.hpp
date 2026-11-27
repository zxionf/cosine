#pragma once

#include "graphics/rectangle.hpp"
#include "ui/node/widget.hpp"

#include <functional>

namespace xel::ui
{
// 遮罩绘制只有这一份实现：Scrim 与 Dialog 共用。
inline void append_scrim(graphics::DrawList& list, Rect bounds, float alpha)
{
    graphics::Rectangle{bounds.pos.x, bounds.pos.y, bounds.size.x, bounds.size.y,
                        {0.0f, 0.0f, 0.0f, alpha}}
        .append_to(list);
}

// 模态遮罩：全屏半透明层，吃掉所有指针与滚轮事件，不参与 Tab 焦点。
// 由 UIRoot 的模态栈创建，z 值始终压在对应窗口下方。
// alpha=0 时可作透明遮罩（如下拉/菜单），layer 可覆写为 Floating。
class Scrim : public Widget
{
public:
    Scrim() = default;
    explicit Scrim(float alpha) : alpha_(alpha) {}

    void set_alpha(float alpha) { alpha_ = alpha; }
    float alpha() const { return alpha_; }

    bool hit_testable() const override { return true; }
    bool focusable() const override { return false; }
    bool fills_parent() const override { return true; }
    // 遮罩必须盖住所有 Floating 弹层，只有 Modal 窗口在其之上；下拉等透明遮罩可覆写为 Floating。
    Layer layer() const override { return layer_; }
    void set_layer(Layer layer) { layer_ = layer; }

    // 尺寸由 UIRoot::layout_child 特判成全屏，这里不参与协商。
    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return local_.size;
    }
    void arrange(Rect slot) override { set_geometry(slot); }

    // 可选：点击/滚轮关闭。默认未接（Window 模态不会因点击遮罩关闭）；
    // 只有命中最上层遮罩时才触发（下层遮罩被 z 覆盖，拿不到事件）。
    void set_on_dismiss(std::function<void()> handler) { on_dismiss_ = std::move(handler); }

    bool on_mouse_down(glm::vec2, int) override
    {
        if (on_dismiss_) on_dismiss_();
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override { return true; }
    // 右键/双击/长按显式吞掉：不转发、不触发关闭。
    bool on_click(glm::vec2, int) override { return true; }
    bool on_right_click(glm::vec2) override { return true; }
    bool on_double_click(glm::vec2, int) override { return true; }
    bool on_long_press(glm::vec2, int) override { return true; }
    bool on_scroll(glm::vec2, glm::vec2) override
    {
        if (on_dismiss_) on_dismiss_();
        return true;
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (alpha_ > 0.0f) append_scrim(list, global_, alpha_);
    }

private:
    float alpha_ = 0.40f;
    Layer layer_ = Layer::Modal;
    std::function<void()> on_dismiss_;
};
} // namespace xel::ui
