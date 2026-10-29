#pragma once

#include "../graphics/rectangle.hpp"
#include "widget.hpp"

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
class Scrim : public Widget
{
public:
    Scrim() = default;
    explicit Scrim(float alpha) : alpha_(alpha) {}

    void set_alpha(float alpha) { alpha_ = alpha; }
    float alpha() const { return alpha_; }

    bool hit_testable() const override { return true; }
    bool focusable() const override { return false; }

    // 尺寸由 UIRoot::layout_child 特判成全屏，这里不参与协商。
    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return local_.size;
    }
    void arrange(Rect slot) override { global_ = slot; }

    bool on_mouse_down(glm::vec2, int) override { return true; }
    bool on_mouse_up(glm::vec2, int) override { return true; }
    bool on_scroll(glm::vec2, glm::vec2) override { return true; }

    void append_to(graphics::DrawList& list) const override
    {
        append_scrim(list, global_, alpha_);
    }

private:
    float alpha_ = 0.40f;
};
} // namespace xel::ui
