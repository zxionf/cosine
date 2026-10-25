#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/rectangle.hpp"
#include "../graphics/triangle.hpp"

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
        float text_w = static_cast<float>(label_.size()) * t.text_size * 0.5f;
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
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float box = t.checkbox_size;
        const float bx = global_.pos.x;
        const float by = global_.pos.y + (global_.size.y - box) * 0.5f;

        graphics::Rectangle background{bx, by, box, box, t.checkbox_bg};
        background.append_to(list);

        const glm::vec4 border_color = hovered_ ? t.focus_ring : t.checkbox_border;
        graphics::Rectangle top{bx, by, box, t.border, border_color};
        graphics::Rectangle bottom{bx, by + box - t.border, box, t.border, border_color};
        graphics::Rectangle left{bx, by, t.border, box, border_color};
        graphics::Rectangle right{bx + box - t.border, by, t.border, box, border_color};
        top.append_to(list);
        bottom.append_to(list);
        left.append_to(list);
        right.append_to(list);

        if (selected_) {
            // 菱形：两个三角形拼成，作为单选标记。
            const float cx = bx + box * 0.5f;
            const float cy = by + box * 0.5f;
            const float r = box * 0.28f;
            graphics::Triangle upper{cx, cy - r, cx + r, cy, cx, cy + r, t.checkbox_fill};
            graphics::Triangle lower{cx, cy - r, cx - r, cy, cx, cy + r, t.checkbox_fill};
            upper.append_to(list);
            lower.append_to(list);
        }

        if (focused_) {
            const float b = 2.0f;
            graphics::Rectangle f_top{bx - b, by - b, box + b * 2.0f, b, t.focus_ring};
            graphics::Rectangle f_bottom{bx - b, by + box, box + b * 2.0f, b, t.focus_ring};
            f_top.append_to(list);
            f_bottom.append_to(list);
        }

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, t.text_size);
            const float tx = bx + box + t.spacing;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(list, tx, ty, t.text_size, label_, enabled_ ? t.text : t.text_disabled);
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; }
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
    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; }

private:
    std::string label_;
    bool selected_ = false;
    bool enabled_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    std::function<void()> handler_;
};
} // namespace xel::ui
