#pragma once

#include "ui/style/theme.hpp"
#include "ui/style/style.hpp"
#include "ui/style/painters.hpp"
#include "ui/node/widget.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/text_debug.hpp"
#include "graphics/box.hpp"

#include <algorithm>
#include <functional>
#include <string>


namespace xel::ui
{
class Button : public Widget
{
public:
    // 语义：让通用代码不必 dynamic_cast 就知道这是按钮。
    WidgetRole semantics_role() const override { return WidgetRole::Button; }
    std::string_view semantics_label() const override { return label_; }
    explicit Button(std::string label, std::function<void()> handler = {})
        : label_{std::move(label)}, handler_{std::move(handler)}
    {}

    void set_label(std::string label) { label_ = std::move(label); mark_layout_dirty(); }
    const std::string& label() const { return label_; }
    void set_on_click(std::function<void()> handler) { handler_ = std::move(handler); }
    void set_enabled(bool enabled) override { input.enabled = enabled; mark_layout_dirty(); }
    bool enabled() const { return is_enabled(); }
    // 伪状态：用于 gallery 与测试展示 pressed 外观，不参与事件流程。
    void set_pressed(bool pressed) { input.pressed = pressed; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();

        float text_width = graphics::FontAtlas::estimate_text_width(label_, t.text_size);
        if (auto* atlas = font_atlas()) {
            text_width = atlas->measure(label_, t.text_size).x;
        }
        const float min_width = t.control_height;   // 短文本也有可点击宽度
        const float width = std::max(min_width, text_width + t.padding * 2.0f);
        return {width, t.control_height};
    }

    // arrange 用 LayoutNode 的默认实现：只接受父容器给的矩形。

    // 基线对齐：按钮文字在控件内垂直居中，按行高 80% 估算基线。
    float baseline_offset_for(float h) const override
    {
        const auto& t = ui_theme();
        auto* atlas = font_atlas();
        const float line = atlas != nullptr ? atlas->line_height(t.text_size) : t.text_size;
        const float top = std::max(0.0f, (h - line) * 0.5f);
        return top + line * 0.8f;
    }

    void append_to(graphics::DrawList& list) const override
    {
        const BoxStyle style = visual_style(StyleRole::Control, input.state());
        const float radius = corner_radius(style.radius);
        const Rect bounds{global_.pos, global_.size};

        // 框 + 边框 + 焦点环走绘制部件（设计 §4.7）。
        paint::box_with_focus(list, bounds, style, radius, ui_theme(), focus_visible());

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_pos = paint::centered_text(list, bounds, label_, style, atlas);
            // debug 是叠加层，不能替代字形本身（否则只剩框子）。
            if (debug_.any()) {
                graphics::append_text_debug(list, *atlas, text_pos.x, text_pos.y, style.text_size,
                                            label_, debug_);
            }
        }
    }

    bool focusable() const override { return true; }

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
        if (handler_) handler_();
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (input.enabled && (key == event::KeyCode::Enter || key == event::KeyCode::KeypadEnter || key == event::KeyCode::Space)) {
            if (handler_) handler_();
            return true;
        }
        return false;
    }

    void set_text_debug(graphics::TextDebugOptions options) { debug_ = options; }
    const graphics::TextDebugOptions& text_debug() const { return debug_; }

private:
    std::string label_;
    std::function<void()> handler_;
    graphics::TextDebugOptions debug_{};
};
} // namespace xel::ui
