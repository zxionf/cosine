#pragma once

#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "ui/geom/layout_metrics.hpp"
#include "ui/style/theme.hpp"
#include "ui/core/collapsible_header.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/node/widget.hpp"

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
    WidgetRole semantics_role() const override { return WidgetRole::Panel; }
    std::string_view semantics_label() const override { return title_; }
    bool semantics_expanded() const override { return expanded_; }
    Section() : Section(std::string{})
    {
    }

    explicit Section(std::string title) : title_(std::move(title))
    {
        auto content = std::make_unique<Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        content->set_spacing(6.0f);
        content->set_cross_align(Align::top_left());
        // UI-49：content_ 现在是真正的子节点（adopt），不再是“自己持有、别人看不见”的裸成员。
        // 旧实现让 Section 的 children_ 恒空，脏标记链在 content_ 处断掉（设计文档 C1）。
        content_ = content.get();
        adopt(std::move(content));
        // Section 横向铺满父级（标题栏/内容卡都是全宽）。
        set_fill(true, false);
    }

    void add(std::unique_ptr<Widget> child) override
    {
        content_->add(std::move(child));
        // measure() 会把 content_ 的缓存高度传给父容器，不失效就一直是旧值。
        content_->invalidate_measure();
        mark_layout_dirty();
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
        // 报“自然尺寸”：宽度 = 标题与内容的最大自然宽（实际宽度由 fill_x 在 arrange 里拉满）。
        const float avail_w = is_bounded(constraints.x) ? constraints.x : local_.size.x;
        const glm::vec2 head = header_.measure({avail_w, kUnbounded});
        float natural_w = head.x;
        float body = 0.0f;
        // 内容高度永远由内容决定（无界），不能传 -1 让子项把百分比算成负值。
        if (expanded_) {
            const glm::vec2 inner = content_->measure({avail_w, kUnbounded});
            body = inner.y;
            natural_w = std::max(natural_w, inner.x);
        }
        return {natural_w, head.y + body};
    }

    void layout() override {}

    void arrange(Rect slot) override
    {
        set_geometry(slot);
        const float hh = header_height();
        // header_ 不在 children_ 里，不会有人替它设 context——必须自己传。
        header_.set_context(context());
        header_.set_title(title_);
        header_.set_expanded(expanded_);
        header_.arrange({slot.pos, {slot.size.x, hh}});
        if (!expanded_) return;
        const float inset = ui_theme().section_inset;
        content_->arrange({{slot.pos.x + inset, slot.pos.y + hh},
                           {std::max(0.0f, slot.size.x - inset * 2.0f),
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
    std::string title_;
    CollapsibleHeader header_;
    bool expanded_ = true;
    // 非拥有指针：所有权在 Widget::children_ 里（UI-49）。
    Vbox* content_ = nullptr;
};
} // namespace xel::ui
