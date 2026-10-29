#pragma once

#include "tab_bar.hpp"
#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
// 带标签栏的页面容器：标签栏固定在上方，内容区显示当前选中的页面。
// 内部 children_[0] 是 TabBar，其余为页面（page(i) == children_[i + 1]）。
class TabView : public Widget
{
public:
    TabView()
    {
        auto bar = std::make_unique<TabBar>();
        bar->set_parent(this);
        bar->set_on_change([this](int index) {
            apply_selection(index);
            if (on_change_) on_change_(index);
        });
        adopt(std::move(bar));
    }

    TabBar& tab_bar() { return *static_cast<TabBar*>(children_.front().get()); }
    const TabBar& tab_bar() const { return *static_cast<const TabBar*>(children_.front().get()); }

    // 返回页面下标（0 起，不含标签栏）。
    int add_page(std::string label, std::unique_ptr<Widget> page)
    {
        if (!page) return -1;
        const int index = static_cast<int>(page_count());
        adopt(std::move(page));
        tab_bar().add_tab(std::move(label));

        // 新增页面默认隐藏，除非它是唯一页面。
        auto* added = pages_owner(index);
        added->set_visible(index == tab_bar().selected());
        mark_layout_dirty();
        return index;
    }

    // 兼容 Widget::add：作为新页面加入，标签名自动生成。
    void add(std::unique_ptr<Widget> child) override
    {
        add_page("Tab " + std::to_string(page_count() + 1), std::move(child));
    }

    void set_page(int index, std::unique_ptr<Widget> page)
    {
        if (!page || index < 0 || index >= static_cast<int>(page_count())) return;
        page->set_parent(this);
        page->set_context(context());
        page->set_visible(index == tab_bar().selected());
        children_[static_cast<std::size_t>(index) + 1] = std::move(page);
        mark_layout_dirty();
    }

    std::size_t page_count() const { return children_.empty() ? 0 : children_.size() - 1; }

    Widget* page(int index) const
    {
        if (index < 0 || index >= static_cast<int>(page_count())) return nullptr;
        return children_[static_cast<std::size_t>(index) + 1].get();
    }

    int selected() const { return tab_bar().selected(); }
    void set_selected(int index) { tab_bar().set_selected(index); }

    void set_on_change(std::function<void(int)> handler) { on_change_ = std::move(handler); }

    // 页面是否填满内容区（默认 true，符合标签页习惯）。
    void set_page_fill(bool fill)
    {
        page_fill_ = fill;
        mark_layout_dirty();
    }
    bool page_fill() const { return page_fill_; }

    void set_context(UIContext* context) override
    {
        Widget::set_context(context);
        if (context) set_padding(context->theme.padding);
    }

    void invalidate_measure() override
    {
        measure_valid_ = false;
        for (const auto& child : children_) child->invalidate_measure();
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        if (measure_valid_ && constraints == cached_constraints_) return cached_measure_;

        const auto& t = ui_theme();
        const float pad = padding_left() + padding_right();
        const float pad_v = padding_top() + padding_bottom();
        const glm::vec2 bar = tab_bar().measure(constraints);

        glm::vec2 content{0.0f, 0.0f};
        for (const auto& child : children_) {
            if (child.get() == &tab_bar()) continue;
            content.x = std::max(content.x, child->measure(constraints).x);
            content.y = std::max(content.y, child->measure(constraints).y);
        }

        cached_constraints_ = constraints;
        cached_measure_ = {
            std::max(bar.x, content.x) + pad,
            bar.y + t.spacing + content.y + pad_v
        };
        measure_valid_ = true;
        return cached_measure_;
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);

        const auto& t = ui_theme();
        const float pad_l = padding_left();
        const float pad_t = padding_top();
        const float inner_w = std::max(0.0f, slot.size.x - pad_l - padding_right());
        const float inner_h = std::max(0.0f, slot.size.y - pad_t - padding_bottom());

        const glm::vec2 bar_size = tab_bar().measure({inner_w, inner_h});
        const float bar_h = std::min(bar_size.y, inner_h);
        tab_bar().arrange({{slot.pos.x + pad_l, slot.pos.y + pad_t}, {inner_w, bar_h}});

        const float content_y = slot.pos.y + pad_t + bar_h + t.spacing;
        const float content_h = std::max(0.0f, slot.pos.y + pad_t + inner_h - content_y);
        const Rect content{{slot.pos.x + pad_l, content_y}, {inner_w, content_h}};

        for (std::size_t i = 1; i < children_.size(); ++i) {
            auto& child = children_[i];
            glm::vec2 size = content.size;
            if (!page_fill_) {
                const glm::vec2 measured = child->measure({content.size.x, content.size.y});
                size = {std::min(measured.x, content.size.x), std::min(measured.y, content.size.y)};
            }
            child->arrange({content.pos, size});
        }
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (is_debug()) debug_rect(list);

        const auto& t = ui_theme();
        if (t.border > 0.0f) {
            const float b = t.border;
            graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b,
                                       global_.size.x, b, t.panel_border};
            graphics::Rectangle right{global_.pos.x + global_.size.x - b, global_.pos.y,
                                      b, global_.size.y, t.panel_border};
            bottom.append_to(list);
            right.append_to(list);
        }

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        tab_bar().append_to(list);
        for (std::size_t i = 1; i < children_.size(); ++i) {
            const auto& child = children_[i];
            if (!child->visible()) continue;
            const Rect page_bounds = child->global_bounds();
            list.push_clip({page_bounds.pos.x, page_bounds.pos.y, page_bounds.size.x, page_bounds.size.y});
            child->append_to(list);
            list.pop_clip();
        }
        list.pop_clip();
    }

    bool hit_testable() const override { return false; }
    bool clips_children() const override { return true; }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

private:
    Widget* pages_owner(int page_index)
    {
        return children_[static_cast<std::size_t>(page_index) + 1].get();
    }

    void apply_selection(int index)
    {
        for (std::size_t i = 1; i < children_.size(); ++i) {
            children_[i]->set_visible(static_cast<int>(i) - 1 == index);
        }
        mark_layout_dirty();
    }

    std::function<void(int)> on_change_;
    glm::vec2 cached_constraints_{};
    glm::vec2 cached_measure_{};
    bool measure_valid_ = false;
    bool page_fill_ = true;
};
} // namespace xel::ui
