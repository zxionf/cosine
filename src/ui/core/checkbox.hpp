#pragma once

#include "ui/style/theme.hpp"
#include "ui/style/style.hpp"
#include "ui/style/painters.hpp"
#include "ui/node/widget.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>


namespace xel::ui
{
class Checkbox : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::CheckBox; }
    std::string_view semantics_label() const override { return label_; }
    explicit Checkbox(std::string label, bool checked = false,
                      std::function<void(bool)> handler = {})
        : label_{std::move(label)}, checked_{checked}, handler_{std::move(handler)}
    {}

    void set_checked(bool checked) { checked_ = checked; }
    bool checked() const { return checked_; }
    void set_enabled(bool enabled) override { input.enabled = enabled; mark_layout_dirty(); }
    bool enabled() const { return is_enabled(); }
    void set_on_change(std::function<void(bool)> handler) { handler_ = std::move(handler); }

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


    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        // 像素对齐：外框、边框与勾选块都落在整像素上，
        // 否则半像素边缘会让勾选块看上去偏心（左右/上下留白不均）。
        const float box = std::round(t.checkbox_size);
        const float bx = std::round(bounds_.pos.x);
        const float by = std::round(bounds_.pos.y + (bounds_.size.y - box) * 0.5f);
        const BoxStyle control = visual_style(StyleRole::Control, input.state());
        const glm::vec4 box_bounds{bx, by, box, box};
        const float radius = std::round(corner_radius(t.radius_small));

        // 框体走 Input 角色（与 TextInput 同为「表单框」）。
        const BoxStyle frame = visual_style(StyleRole::Input, input.state());
        // 框体 + 边框 + 焦点环走绘制部件（边框宽度取主题的 t.border）。
        BoxStyle frame_box = frame;
        frame_box.border_width = t.border;
        paint::box_with_focus(list, Rect{{box_bounds.x, box_bounds.y}, {box_bounds.z, box_bounds.w}},
                              frame_box, radius, t, focus_visible());

        if (checked_) {
            // inset 取整：勾选块与外框同心，四边留白完全相等且边界落在整像素。
            const float inset = std::round(box * 0.25f);
            const float marker = box - inset * 2.0f;
            // 过渡按角色分槽，因此可以再请求 Accent（不会污染 label 的 Control 过渡），
            // 勾选块因此也有了颜色过渡。
            const BoxStyle accent = visual_style(StyleRole::Accent, input.state());
            graphics::append_rounded_rect(list,
                                          {bx + inset, by + inset, marker, marker},
                                          radius * 0.5f,
                                          accent.fill);
        }

        if (auto* atlas = font_atlas()) {
            const glm::vec2 size = atlas->measure(label_, control.text_size);
            const float tx = bx + box + t.spacing;
            const float ty = bounds_.pos.y + (bounds_.size.y - size.y) * 0.5f;
            atlas->append_text(list, tx, ty, control.text_size, label_, control.text);
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
        checked_ = !checked_;
        if (handler_) handler_(checked_);
        return true;
    }
    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (input.enabled && (key == event::KeyCode::Space || key == event::KeyCode::Enter || key == event::KeyCode::KeypadEnter)) {
            checked_ = !checked_;
            if (handler_) handler_(checked_);
            return true;
        }
        return false;
    }
    void on_focus_lost() override { input.pressed = false; Widget::on_focus_lost(); }

private:
    std::string label_;
    bool checked_ = false;
    std::function<void(bool)> handler_;
};
} // namespace xel::ui
