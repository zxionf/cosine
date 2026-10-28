#pragma once

#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "theme.hpp"
#include "vbox.hpp"
#include "widget.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace xel::ui
{
// DevTools 风格的折叠分组：标题行（折叠三角 + 标题）点击即展开/收起。
// 内容放在内部的 Vbox 里，add() 直接转发过去。
class Section : public Widget
{
public:
    Section() : Section(std::string{})
    {
    }

    explicit Section(std::string title) : title_(std::move(title))
    {
        content_ = std::make_unique<Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        content_->set_spacing(6.0f);
        content_->set_cross_align(Align::top_left());
    }

    void add(std::unique_ptr<Widget> child) override
    {
        content_->add(std::move(child));
        // measure() 会把 content_ 的缓存高度传给父容器，不失效就一直是旧值。
        content_->invalidate_measure();
        mark_layout_dirty();
    }

    const std::vector<std::unique_ptr<Widget>>& children() const override
    {
        return content_->children();
    }

    void set_title(std::string title)
    {
        title_ = std::move(title);
        mark_layout_dirty();
    }

    void set_expanded(bool on)
    {
        expanded_ = on;
        mark_layout_dirty();
    }
    bool expanded() const { return expanded_; }
    void set_spacing(float spacing) { content_->set_spacing(spacing); }

    // 切换页时整块重建用：清掉内容但保留分组本身。
    void clear()
    {
        content_->clear_children();
        content_->invalidate_measure();
        mark_layout_dirty();
    }

    float header_height() const
    {
        const auto& t = ui_theme();
        return std::max(t.control_height * 0.72f, t.text_size + 10.0f);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        float width = constraints.x;
        if (width < 0.0f) width = local_.size.x;
        float body = 0.0f;
        if (expanded_) body = content_->measure({width, -1.0f}).y;
        return {width, header_height() + body};
    }

    void layout() override {}

    void arrange(Rect slot) override
    {
        set_pos(slot.pos);
        set_size(slot.size);
        global_ = slot;
        if (!expanded_) return;
        const float hh = header_height();
        content_->arrange({{slot.pos.x + kContentInset, slot.pos.y + hh},
                           {std::max(0.0f, slot.size.x - kContentInset * 2.0f),
                            std::max(0.0f, slot.size.y - hh)}});
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float hh = header_height();
        graphics::Rectangle{global_.pos.x, global_.pos.y, global_.size.x, hh, t.panel_bg}
            .append_to(list);

        // 折叠三角：展开朝下（V），收起朝右（>）。
        const float cy = global_.pos.y + hh * 0.5f;
        const glm::vec4 glyph = t.text_muted;
        const float ax = global_.pos.x + 8.0f;
        if (expanded_) {
            graphics::Line{ax, cy - 4.0f, ax + 7.0f, cy + 3.0f, 1.5f, glyph}.append_to(list);
            graphics::Line{ax + 7.0f, cy + 3.0f, ax + 14.0f, cy - 4.0f, 1.5f, glyph}.append_to(list);
        } else {
            graphics::Line{ax, cy - 4.0f, ax + 7.0f, cy, 1.5f, glyph}.append_to(list);
            graphics::Line{ax + 7.0f, cy, ax, cy + 4.0f, 1.5f, glyph}.append_to(list);
        }

        if (auto* atlas = font_atlas()) {
            const float line_h = atlas->line_height(t.text_size);
            atlas->append_text(list, global_.pos.x + 30.0f, global_.pos.y + (hh - line_h) * 0.5f,
                               t.text_size, title_, t.text);
        }
        if (expanded_) content_->append_to(list);
    }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (pos.y >= global_.pos.y + header_height()) return false;
        expanded_ = !expanded_;
        mark_layout_dirty();
        if (auto* p = parent()) p->mark_layout_dirty();
        return true;
    }

    bool hit_testable() const override { return true; }

private:
    static constexpr float kContentInset = 12.0f;

    std::string title_;
    bool expanded_ = true;
    std::unique_ptr<Vbox> content_;
};
} // namespace xel::ui
