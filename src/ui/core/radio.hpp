#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"
#include "graphics/triangle.hpp"

#include <algorithm>
#include <functional>
#include <string>

namespace xel::ui
{
// 单选按钮：选中时显示菱形标记，与 Checkbox 的方形标记区分。
class Radio : public Widget
{
public:
    explicit Radio(std::string label, bool selected = false,
                   std::function<void()> handler = {})
        : label_{std::move(label)}, selected_{selected}, handler_{std::move(handler)}
    {}

    void set_label(std::string label) { label_ = std::move(label); mark_layout_dirty(); }
    const std::string& label() const { return label_; }

    bool selected() const { return selected_; }
    void set_selected(bool selected) { selected_ = selected; }
    void select()
    {
        if (selected_) return;
        selected_ = true;
        if (handler_) handler_();
    }

    void set_on_select(std::function<void()> handler) { handler_ = std::move(handler); }
    void set_enabled(bool enabled) { enabled_ = enabled; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        float text_w = graphics::FontAtlas::estimate_text_width(label_, t.text_size);
        if (auto* atlas = font_atlas()) {
            text_w = atlas->measure(label_, t.text_size).x;
        }
        return {
            t.checkbox_size + (label_.empty() ? 0.0f : t.spacing + text_w),
            std::max(t.checkbox_size, t.text_size)
        };
    }

    void arrange(Rect slot) override
    {
        set_geometry(slot);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float box = t.checkbox_size;
        const float bx = global_.pos.x;
        const float by = global_.pos.y + (global_.size.y - box) * 0.5f;
        const BoxStyle control = visual_style(StyleRole::Control, control_state(enabled_, is_hovered(), false));
        const glm::vec4 box_bounds{bx, by, box, box};
        // 单选标记：modern 为圆形（radius_pill 被 clamp 到半宽），classic 为方形。
        const float radius = corner_radius(t.radius_pill);

        // UI-40：框体走 Input 角色（与 Checkbox 一致），不再直读 checkbox_bg/border。
        const BoxStyle frame = visual_style(StyleRole::Input,
                                            control_state(enabled_, is_hovered(), false));
        graphics::append_rounded_rect(list, box_bounds, radius, frame.fill);
        graphics::append_rounded_ring(list, box_bounds, radius, t.border, frame.border);

        if (selected_) {
            // 菱形：两个三角形拼成，作为单选标记。
            const float cx = bx + box * 0.5f;
            const float cy = by + box * 0.5f;
            const float r = box * 0.28f;
            // UI-38 起过渡按角色分槽，这里可以安全地再请求 Accent（不再污染 label 的 Control 过渡）。
            const BoxStyle accent = visual_style(StyleRole::Accent,
                                                 enabled_ ? ControlState::Normal : ControlState::Disabled);
            const glm::vec4 marker = accent.fill;
            graphics::Triangle upper{cx, cy - r, cx + r, cy, cx, cy + r, marker};
            graphics::Triangle lower{cx, cy - r, cx - r, cy, cx, cy + r, marker};
            upper.append_to(list);
            lower.append_to(list);
        }

        append_focus_ring(list, box_bounds, radius);

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, control.text_size);
            const float tx = bx + box + t.spacing;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(list, tx, ty, control.text_size, label_, control.text);
        }
    }

    bool focusable() const override { return true; }

    bool on_click(glm::vec2, int) override
    {
        if (!enabled_) return false;
        select();
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!enabled_) return false;
        if (key == event::KeyCode::Space || key == event::KeyCode::Enter || key == event::KeyCode::KeypadEnter) {
            select();
            return true;
        }
        return false;
    }
    void on_focus_lost() override { Widget::on_focus_lost(); }

private:
    std::string label_;
    bool selected_ = false;
    bool enabled_ = true;
    std::function<void()> handler_;
};
} // namespace xel::ui
