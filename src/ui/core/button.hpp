#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
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
    explicit Button(std::string label, std::function<void()> handler = {})
        : label_{std::move(label)}, handler_{std::move(handler)}
    {}

    void set_label(std::string label) { label_ = std::move(label); mark_layout_dirty(); }
    const std::string& label() const { return label_; }
    void set_on_click(std::function<void()> handler) { handler_ = std::move(handler); }
    void set_enabled(bool enabled) { enabled_ = enabled; mark_layout_dirty(); }
    bool enabled() const { return enabled_; }
    // 伪状态：用于 gallery 与测试展示 pressed 外观，不参与事件流程。
    void set_pressed(bool pressed) { pressed_ = pressed; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        glm::vec2 text_size{static_cast<float>(label_.size()) * t.text_size * 0.5f, t.text_size};
        if (auto* atlas = font_atlas()) {
            text_size = atlas->measure(label_, t.text_size);
        }
        return {
            text_size.x + t.padding * 2.0f,
            std::max(t.control_height, text_size.y + t.padding * 2.0f)
        };
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const BoxStyle style = visual_style(StyleRole::Control, control_state(enabled_, is_hovered(), pressed_));
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const float radius = corner_radius(style.radius);

        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, style.border);
        }
        append_focus_ring(list, bounds, radius);

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, style.text_size);
            const float tx = global_.pos.x + (global_.size.x - text_size.x) * 0.5f;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(list, tx, ty, style.text_size, label_, style.text);
            // debug 是叠加层，不能替代字形本身（否则只剩框子）。
            if (debug_.any()) {
                graphics::append_text_debug(list, *atlas, tx, ty, style.text_size, label_, debug_);
            }
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_leave() override { pressed_ = false; }
    bool on_mouse_down(glm::vec2, int) override
    {
        if (!enabled_) return false;
        pressed_ = true;
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override
    {
        pressed_ = false;
        return true;
    }
    bool on_click(glm::vec2, int) override
    {
        if (!enabled_) return false;
        if (handler_) handler_();
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (enabled_ && (key == event::KeyCode::Enter || key == event::KeyCode::KeypadEnter || key == event::KeyCode::Space)) {
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
    bool enabled_ = true;
    bool pressed_ = false;
    graphics::TextDebugOptions debug_{};
};
} // namespace xel::ui
