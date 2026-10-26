#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/box.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace xel::ui
{
// 水平标签栏：点击或左右方向键切换，选中项下方有强调线。
class TabBar : public Widget
{
public:
    TabBar() = default;

    void set_tabs(std::vector<std::string> tabs)
    {
        tabs_ = std::move(tabs);
        if (tabs_.empty()) {
            selected_ = -1;
        } else if (selected_ < 0 || selected_ >= static_cast<int>(tabs_.size())) {
            selected_ = 0;
        }
        mark_layout_dirty();
    }

    void add_tab(std::string label)
    {
        tabs_.push_back(std::move(label));
        if (selected_ < 0) selected_ = 0;
        mark_layout_dirty();
    }

    const std::vector<std::string>& tabs() const { return tabs_; }
    int selected() const { return selected_; }

    void set_selected(int index, bool notify = true)
    {
        if (tabs_.empty()) return;
        const int clamped = std::clamp(index, 0, static_cast<int>(tabs_.size()) - 1);
        if (clamped == selected_) return;
        selected_ = clamped;
        if (notify && on_change_) on_change_(selected_);
    }

    void set_on_change(std::function<void(int)> handler) { on_change_ = std::move(handler); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        float width = 0.0f;
        for (const auto& tab : tabs_) {
            width += tab_width(tab) + t.spacing;
        }
        if (!tabs_.empty()) width -= t.spacing;
        return {width, t.control_height};
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
        const float top = global_.pos.y;
        const float height = global_.size.y;
        const float radius = corner_radius(t.radius_small);

        float x = global_.pos.x;
        for (std::size_t i = 0; i < tabs_.size(); ++i) {
            const float width = tab_width(tabs_[i]);
            const bool active = static_cast<int>(i) == selected_;
            const bool hovered = static_cast<int>(i) == hovered_ && !active;
            const BoxStyle style = t.style(StyleRole::Control,
                                           active ? ControlState::Pressed
                                                  : hovered ? ControlState::Hovered
                                                            : ControlState::Normal);

            graphics::append_rounded_rect(list, {x, top, width, height}, radius, style.fill);

            if (active) {
                graphics::append_rounded_rect(list, {x, top + height - 2.0f, width, 2.0f}, radius, t.accent);
            }

            if (auto* atlas = font_atlas()) {
                const glm::vec2 text_size = atlas->measure(tabs_[i], style.text_size);
                atlas->append_text(list,
                                   x + (width - text_size.x) * 0.5f,
                                   top + (height - text_size.y) * 0.5f,
                                   style.text_size, tabs_[i], style.text);
            }

            x += width + t.spacing;
        }

        append_focus_ring(list, {global_.pos.x, top, global_.size.x, height}, radius);
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = index_at_last_mouse_; }
    void on_mouse_leave() override { hovered_ = -1; }
    void on_hover(glm::vec2 pos) override
    {
        hovered_ = index_at(pos);
        index_at_last_mouse_ = hovered_;
    }

    bool on_click(glm::vec2 pos, int) override
    {
        if (tabs_.empty()) return false;
        const int index = index_at(pos);
        if (index < 0) return false;
        set_selected(index);
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (tabs_.empty()) return false;
        if (key == event::KeyCode::Left) {
            set_selected(selected_ - 1);
            return true;
        }
        if (key == event::KeyCode::Right) {
            set_selected(selected_ + 1);
            return true;
        }
        if (key == event::KeyCode::Home) {
            set_selected(0);
            return true;
        }
        if (key == event::KeyCode::End) {
            set_selected(static_cast<int>(tabs_.size()) - 1);
            return true;
        }
        return false;
    }

    void on_focus_lost() override { Widget::on_focus_lost(); }

private:
    float text_width(const std::string& text) const
    {
        const auto& t = ui_theme();
        if (auto* atlas = font_atlas()) {
            return atlas->measure(text, t.text_size).x;
        }
        return static_cast<float>(text.size()) * t.text_size * 0.5f;
    }

    float tab_width(const std::string& text) const
    {
        return text_width(text) + ui_theme().padding * 2.0f;
    }

    int index_at(glm::vec2 pos) const
    {
        if (pos.y < global_.pos.y || pos.y > global_.pos.y + global_.size.y) return -1;
        const auto& t = ui_theme();
        float x = global_.pos.x;
        for (std::size_t i = 0; i < tabs_.size(); ++i) {
            const float width = tab_width(tabs_[i]);
            if (pos.x >= x && pos.x <= x + width) return static_cast<int>(i);
            x += width + t.spacing;
        }
        return -1;
    }

    std::vector<std::string> tabs_;
    int selected_ = -1;
    int hovered_ = -1;
    int index_at_last_mouse_ = -1;
    std::function<void(int)> on_change_;
};
} // namespace xel::ui
