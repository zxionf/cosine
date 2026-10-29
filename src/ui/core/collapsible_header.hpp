#pragma once

#include "graphics/font_atlas.hpp"
#include "graphics/line.hpp"
#include "graphics/rectangle.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace xel::ui
{
// 可折叠标题条：panel_bg 底 + 折叠三角 + 标题。
// 只负责「长什么样」与展开状态；命中与切换由宿主控件处理
// （Section / Window 的标题栏各有自己的命中语义，套在一起反而更绕）。
class CollapsibleHeader : public Widget
{
public:
    explicit CollapsibleHeader(std::string title = {}) : title_(std::move(title)) {}

    void set_title(std::string title)
    {
        title_ = std::move(title);
        mark_layout_dirty();
    }
    const std::string& title() const { return title_; }

    void set_expanded(bool expanded) { expanded_ = expanded; }
    bool expanded() const { return expanded_; }

    // 0 = 跟随主题（control_height * 0.72，且不低于 text_size + 10）。
    void set_height(float height)
    {
        height_hint_ = std::max(0.0f, height);
        mark_layout_dirty();
    }
    float height() const
    {
        if (height_hint_ > 0.0f) return height_hint_;
        const auto& t = ui_theme();
        return std::max(t.control_height * 0.72f, t.text_size + 10.0f);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        float width = constraints.x;
        if (width < 0.0f) width = local_.size.x;
        return {width, height()};
    }

    void arrange(Rect slot) override
    {
        set_pos(slot.pos);
        set_size(slot.size);
        global_ = {slot.pos, {slot.size.x, height()}};
    }

    bool hit_testable() const override { return false; }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float hh = height();
        graphics::Rectangle{global_.pos.x, global_.pos.y, global_.size.x, hh, t.panel_bg}
            .append_to(list);

        // 折叠三角：展开朝下（V），收起朝右（>）。
        const float cy = global_.pos.y + hh * 0.5f;
        const glm::vec4 glyph = t.text_muted;
        const float ax = global_.pos.x + 8.0f;
        if (expanded_) {
            graphics::Line{ax, cy - 4.0f, ax + 7.0f, cy + 3.0f, 1.5f, glyph}.append_to(list);
            graphics::Line{ax + 7.0f, cy + 3.0f, ax + 14.0f, cy - 4.0f, 1.5f, glyph}
                .append_to(list);
        } else {
            graphics::Line{ax, cy - 4.0f, ax + 7.0f, cy, 1.5f, glyph}.append_to(list);
            graphics::Line{ax + 7.0f, cy, ax, cy + 4.0f, 1.5f, glyph}.append_to(list);
        }

        if (auto* atlas = font_atlas()) {
            const float text_y = atlas->centered_text_y(t.text_size, global_.pos.y, hh);
            atlas->append_text(list, global_.pos.x + 30.0f, text_y, t.text_size, title_,
                               t.text);
        }
    }

private:
    std::string title_;
    float height_hint_ = 0.0f;
    bool expanded_ = true;
};
} // namespace xel::ui
