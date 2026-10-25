#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/text_debug.hpp"
#include "../graphics/utf8.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace xel::ui
{
enum class TextAlign
{
    Left,
    Center,
    Right,
};

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
    bool wrap() const { return wrap_; }
    void set_max_lines(int max_lines) { max_lines_ = max_lines; mark_layout_dirty(); }
    void set_ellipsis(bool ellipsis) { ellipsis_ = ellipsis; mark_layout_dirty(); }

    void set_text_align(TextAlign align) { text_align_ = align; }
    TextAlign text_align() const { return text_align_; }

    // 像素对齐：把绘制原点对齐到整数像素，避免线性采样造成的半像素模糊。
    void set_pixel_snap(bool snap) { pixel_snap_ = snap; }
    bool pixel_snap() const { return pixel_snap_; }

    // 行间额外间距（仅在多行/换行时生效）。
    void set_line_spacing(float spacing)
    {
        line_spacing_ = std::max(0.0f, spacing);
        mark_layout_dirty();
    }
    float line_spacing() const { return line_spacing_; }

    void set_text_debug(graphics::TextDebugOptions options) { debug_ = options; }
    const graphics::TextDebugOptions& text_debug() const { return debug_; }
    void set_text_debug_color(glm::vec4 color) { debug_.color = color; }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        const float size = text_size();
        if (auto* atlas = font_atlas()) {
            if (wrap_ && constraints.x > 0.0f && constraints.x < 1.0e8f) {
                // 换行宽度取「可用宽度」与控件最大宽度中较小者，
                // 这样测量与 arrange 后的实际宽度一致。
                float wrap_width = constraints.x;
                if (max_size().x < wrap_width) wrap_width = max_size().x;
                glm::vec2 measured =
                    atlas->measure_wrapped(text_, size, wrap_width, line_spacing_);
                if (max_lines_ > 0) {
                    const float line_h = atlas->line_step(size, line_spacing_);
                    measured.y = std::min(measured.y, static_cast<float>(max_lines_) * line_h);
                }
                return measured;
            }
            return atlas->measure(text_, size, line_spacing_);
        }

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
        auto* atlas = font_atlas();
        if (!atlas) return;

        const float size = text_size();
        const float text_w = wrap_ ? global_.size.x
                                   : atlas->measure(text_, size, line_spacing_).x;
        const float offset_x = align_offset(text_w, global_.size.x);

        float tx = global_.pos.x + offset_x;
        float ty = global_.pos.y;
        if (pixel_snap_) {
            tx = std::round(tx);
            ty = std::round(ty);
        }

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        if (wrap_) {
            atlas->append_text_wrapped(list, tx, ty, size, text_, text_color(),
                                       global_.size.x, max_lines_, ellipsis_,
                                       {}, line_spacing_);
        } else {
            atlas->append_text(list, tx, ty, size, text_, text_color(), {},
                               line_spacing_);
        }

        if (debug_.any()) {
            graphics::TextDebugOptions options = debug_;
            options.line_spacing = line_spacing_;
            if (wrap_) {
                options.wrap_width = global_.size.x;
                options.max_lines = max_lines_;
            }
            graphics::append_text_debug(list, *atlas, tx, ty, size, text_, options);
        }
        list.pop_clip();
    }
    bool hit_testable() const override { return false; }

private:
    float align_offset(float text_w, float available_w) const
    {
        if (available_w <= text_w) return 0.0f;
        switch (text_align_) {
            case TextAlign::Center: return (available_w - text_w) * 0.5f;
            case TextAlign::Right: return available_w - text_w;
            case TextAlign::Left: break;
        }
        return 0.0f;
    }

    std::string text_;
    float size_ = 0.0f;
    glm::vec4 color_{};
    bool wrap_ = false;
    int max_lines_ = 0;
    bool ellipsis_ = false;
    TextAlign text_align_ = TextAlign::Left;
    bool pixel_snap_ = true;
    float line_spacing_ = 0.0f;
    graphics::TextDebugOptions debug_{};
};
} // namespace xel::ui
