#pragma once

#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "theme.hpp"
#include "collapsible_header.hpp"
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

    float header_height() const { return header_.height(); }

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
        const float hh = header_height();
        header_.set_title(title_);
        header_.set_expanded(expanded_);
        header_.arrange({slot.pos, {slot.size.x, hh}});
        if (!expanded_) return;
        content_->arrange({{slot.pos.x + kContentInset, slot.pos.y + hh},
                           {std::max(0.0f, slot.size.x - kContentInset * 2.0f),
                            std::max(0.0f, slot.size.y - hh)}});
    }

    void append_to(graphics::DrawList& list) const override
    {
        header_.append_to(list);
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
    CollapsibleHeader header_;
    bool expanded_ = true;
    std::unique_ptr<Vbox> content_;
};
} // namespace xel::ui
