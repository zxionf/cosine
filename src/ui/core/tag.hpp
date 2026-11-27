#pragma once

#include "ui/node/leaf_node.hpp"
#include "ui/style/painters.hpp"
#include "ui/style/style.hpp"
#include "ui/style/theme.hpp"

#include <functional>
#include <string>
#include <utility>

namespace xel::ui
{
// Tag：一个「新控件到底要写多少行」的实证。
//
// 它的需求覆盖了典型交互叶子：可点、可禁用、有 hover/pressed、带焦点环、文字居中。
// 这些全部由基类与绘制部件提供，本类只剩「我是什么」：
//   - 尺寸：首选尺寸（构造期定），measure/arrange 由 LeafNode 给
//   - 外观：StyleRole::Control 的一帧样式（painters 画框 + 焦点环 + 文字）
//   - 交互：pressed 状态、点击回调、Enter/Space 触发
//   - 语义：role/label 供通用代码查询
//
class Tag : public LeafNode
{
public:
    explicit Tag(std::string label, std::function<void()> on_click = {})
        : label_{std::move(label)}, on_click_{std::move(on_click)}
    {
        set_preferred_size({label_.size() * ui_theme().text_size * 0.6f + ui_theme().padding * 2.0f,
                            ui_theme().control_height});
    }

    void set_label(std::string label)
    {
        label_ = std::move(label);
        mark_layout_dirty();
    }

    WidgetRole semantics_role() const override { return WidgetRole::Button; }
    std::string_view semantics_label() const override { return label_; }
    bool focusable() const override { return true; }

    void set_on_click(std::function<void()> handler) { on_click_ = std::move(handler); }

    void append_to(graphics::DrawList& list) const override
    {
        const BoxStyle style = visual_style(StyleRole::Control, input.state());
        const Rect bounds{global_.pos, global_.size};
        const float radius = corner_radius(style.radius);
        paint::box_with_focus(list, bounds, style, radius, ui_theme(), focus_visible());
        paint::centered_text(list, bounds, label_, style, font_atlas());
    }

    void on_mouse_leave() override { input.pressed = false; }
    bool on_mouse_down(glm::vec2, int) override
    {
        if (!is_enabled()) return false;
        input.pressed = true;
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override
    {
        input.pressed = false;
        return true;
    }
    bool on_click(glm::vec2, int) override
    {
        if (!is_enabled()) return false;
        if (on_click_) on_click_();
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!is_enabled()) return false;
        if (key != event::KeyCode::Enter && key != event::KeyCode::Space) return false;
        if (on_click_) on_click_();
        return true;
    }

private:
    std::string label_;
    std::function<void()> on_click_;
};
} // namespace xel::ui
