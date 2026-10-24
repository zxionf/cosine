#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <string>

#include <GLFW/glfw3.h>

namespace xel::ui
{
class Button : public Widget
{
public:
    explicit Button(std::string label, std::function<void()> handler = {})
        : label_{std::move(label)}, handler_{std::move(handler)}
    {}

    void set_label(std::string label) { label_ = std::move(label); mark_layout_dirty(); }
    void set_on_click(std::function<void()> handler) { handler_ = std::move(handler); }
    void set_enabled(bool enabled) { enabled_ = enabled; mark_layout_dirty(); }
    bool enabled() const { return enabled_; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = theme();
        glm::vec2 text_size{static_cast<float>(label_.size()) * t.text_size * 0.5f, t.text_size};
        if (auto* atlas = graphics::active_font_atlas()) {
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

    void append_to(graphics::DrawBatch& batch) const override
    {
        const auto& t = theme();

        glm::vec4 bg = t.button_bg;
        if (!enabled_) bg = t.button_disabled;
        else if (pressed_) bg = t.button_pressed;
        else if (hovered_) bg = t.button_hover;

        graphics::Rectangle background{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y, bg};
        background.append_to(batch);

        if (focused_) {
            const float b = 2.0f;
            graphics::Rectangle top   {global_.pos.x, global_.pos.y, global_.size.x, b, t.focus_ring};
            graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b, global_.size.x, b, t.focus_ring};
            graphics::Rectangle left  {global_.pos.x, global_.pos.y, b, global_.size.y, t.focus_ring};
            graphics::Rectangle right {global_.pos.x + global_.size.x - b, global_.pos.y, b, global_.size.y, t.focus_ring};
            top.append_to(batch);
            bottom.append_to(batch);
            left.append_to(batch);
            right.append_to(batch);
        }

        if (auto* atlas = graphics::active_font_atlas()) {
            const glm::vec2 text_size = atlas->measure(label_, t.text_size);
            const float tx = global_.pos.x + (global_.size.x - text_size.x) * 0.5f;
            const float ty = global_.pos.y + (global_.size.y - text_size.y) * 0.5f;
            atlas->append_text(batch, tx, ty, t.text_size, label_, enabled_ ? t.text : t.text_disabled);
        }
    }

    bool focusable() const override { return true; }

    void on_mouse_enter() override { hovered_ = true; }
    void on_mouse_leave() override { hovered_ = false; pressed_ = false; }
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
    bool on_key_down(int key, int) override
    {
        if (enabled_ && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_SPACE)) {
            if (handler_) handler_();
            return true;
        }
        return false;
    }
    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; }

private:
    std::string label_;
    std::function<void()> handler_;
    bool enabled_ = true;
    bool hovered_ = false;
    bool pressed_ = false;
    bool focused_ = false;
};
} // namespace xel::ui
