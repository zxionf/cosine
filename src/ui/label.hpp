#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"

#include <string>

namespace xel::ui
{
class Label : public Widget
{
public:
    explicit Label(std::string text, float size = 0.0f, glm::vec4 color = glm::vec4{0.0f})
        : text_{std::move(text)}, size_{size}, color_{color}
    {
        if (size_ <= 0.0f) size_ = theme().text_size;
        if (color_.a == 0.0f) color_ = theme().text;
    }

    void set_text(std::string text)
    {
        if (text_ == text) return;
        text_ = std::move(text);
        mark_layout_dirty();
    }
    const std::string& text() const { return text_; }
    void set_text_size(float size) { size_ = size; mark_layout_dirty(); }
    void set_text_color(glm::vec4 color) { color_ = color; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        if (auto* atlas = graphics::active_font_atlas()) {
            return atlas->measure(text_, size_);
        }
        return {static_cast<float>(text_.size()) * size_ * 0.5f, size_};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawBatch& batch) const override
    {
        if (auto* atlas = graphics::active_font_atlas()) {
            atlas->append_text(batch, x(), y(), size_, text_, color_);
        }
    }

    bool hit_testable() const override { return false; }

private:
    std::string text_;
    float size_ = 0.0f;
    glm::vec4 color_{};
};
} // namespace xel::ui
