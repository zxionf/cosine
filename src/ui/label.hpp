#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/utf8.hpp"

#include <algorithm>
#include <string>

namespace xel::ui
{
class Label : public Widget
{
public:
    explicit Label(std::string text, float size = 0.0f, glm::vec4 color = glm::vec4{0.0f})
        : text_{std::move(text)}, size_{size}, color_{color}
    {}

    float text_size() const { return size_ > 0.0f ? size_ : ui_theme().text_size; }
    glm::vec4 text_color() const { return color_.a > 0.0f ? color_ : ui_theme().text; }

    void set_text(std::string text)
    {
        if (text_ == text) return;
        text_ = std::move(text);
        mark_layout_dirty();
    }
    const std::string& text() const { return text_; }
    void set_text_size(float size) { size_ = size; mark_layout_dirty(); }
    void set_text_color(glm::vec4 color) { color_ = color; }
    void set_wrap(bool wrap) { wrap_ = wrap; mark_layout_dirty(); }
    void set_max_lines(int max_lines) { max_lines_ = max_lines; mark_layout_dirty(); }
    void set_ellipsis(bool ellipsis) { ellipsis_ = ellipsis; mark_layout_dirty(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        if (auto* atlas = font_atlas()) {
            const float size = text_size();
            if (wrap_ && constraints.x > 0.0f && constraints.x < 1.0e8f) {
                glm::vec2 measured = atlas->measure_wrapped(text_, size, constraints.x);
                if (max_lines_ > 0) {
                    measured.y = std::min(measured.y, static_cast<float>(max_lines_) * size);
                }
                return measured;
            }
            return atlas->measure(text_, size);
        }

        const float size = text_size();
        const float advance = size * 0.5f;
        const std::size_t count = graphics::utf8_codepoint_count(text_);
        return {static_cast<float>(count) * advance, size};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (auto* atlas = font_atlas()) {
            list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
            if (wrap_) {
                atlas->append_text_wrapped(list, x(), y(), text_size(), text_, text_color(),
                                           global_.size.x, max_lines_, ellipsis_);
            } else {
                atlas->append_text(list, x(), y(), text_size(), text_, text_color());
            }
            list.pop_clip();
        }
    }
    bool hit_testable() const override { return false; }

private:
    std::string text_;
    float size_ = 0.0f;
    glm::vec4 color_{};
    bool wrap_ = false;
    int max_lines_ = 0;
    bool ellipsis_ = false;
};
} // namespace xel::ui
