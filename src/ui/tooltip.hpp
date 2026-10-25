#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/rectangle.hpp"

#include <string>

namespace xel::ui
{
class Tooltip : public Widget
{
public:
    explicit Tooltip(std::string text = {})
        : text_{std::move(text)}
    {
        set_visible(false);
    }

    void set_text(std::string text) { text_ = std::move(text); mark_layout_dirty(); }
    const std::string& text() const { return text_; }

    void show_at(glm::vec2 pos)
    {
        set_pos(pos);
        set_visible(true);
    }

    void hide() { set_visible(false); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        glm::vec2 text_size{static_cast<float>(text_.size()) * t.small_text_size * 0.5f, t.small_text_size};
        if (auto* atlas = font_atlas()) {
            text_size = atlas->measure(text_, t.small_text_size);
        }
        return {text_size.x + t.padding * 2.0f, text_size.y + t.padding};
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
        graphics::Rectangle background{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y, t.tooltip_bg};
        background.append_to(list);

        if (auto* atlas = font_atlas()) {
            atlas->append_text(list, global_.pos.x + t.padding, global_.pos.y + t.padding * 0.5f,
                               t.small_text_size, text_, t.text);
        }
    }

    bool hit_testable() const override { return false; }

private:
    std::string text_;
};
} // namespace xel::ui
