#pragma once

#include "ui/core/tab_bar.hpp"
#include "ui/layout/layout_utils.hpp"
#include "ui/system/uicontext.hpp"
#include "ui/style/theme.hpp"
#include "ui/node/widget.hpp"
#include "graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
// 带标签栏的页面容器：标签栏固定在上方，内容区显示当前选中的页面。
// 内部 children()[0] 是 TabBar，其余为页面（page(i) == child_at(i + 1)）。
class TabView : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::TabView; }
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

    TabBar& tab_bar() { return *static_cast<TabBar*>(child_at(0)); }
    const TabBar& tab_bar() const { return *static_cast<const TabBar*>(child_at(0)); }

    // 返回页面下标（0 起，不含标签栏）。
    int add_page(std::string label, std::unique_ptr<Node> page)
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
    void add(std::unique_ptr<Node> child) override
    {
        add_page("Tab " + std::to_string(page_count() + 1), std::move(child));
    }

    void set_page(int index, std::unique_ptr<Node> page)
    {
        if (!page || index < 0 || index >= static_cast<int>(page_count())) return;
        page->set_parent(this);
        page->set_context(context());
        page->set_visible(index == tab_bar().selected());
        replace_child(static_cast<std::size_t>(index) + 1, std::move(page));
        mark_layout_dirty();
    }

    std::size_t page_count() const { return child_count() == 0 ? 0 : child_count() - 1; }

    LayoutNode* page(int index) const
    {
        if (index < 0 || index >= static_cast<int>(page_count())) return nullptr;
        return child_at(static_cast<std::size_t>(index) + 1);
    }

    int selected() const { return tab_bar().selected(); }
    void set_selected(int index) { tab_bar().set_selected(index); }

    void set_on_change(std::function<void(int)> handler) { on_change_ = std::move(handler); }

    enum class BarPosition
    {
        Top,
        Bottom,
        Left,
        Right,
    };

    // 标签栏位置：Left/Right 会把 TabBar 切到竖向。
    void set_bar_position(BarPosition position)
    {
        if (bar_position_ == position) return;
        bar_position_ = position;
        tab_bar().set_orientation(position == BarPosition::Left || position == BarPosition::Right
                                      ? TabBar::Orientation::Vertical
                                      : TabBar::Orientation::Horizontal);
        mark_layout_dirty();
    }
    BarPosition bar_position() const { return bar_position_; }

    // 页面是否填满内容区（默认 true，符合标签页习惯）。
    void set_page_fill(bool fill)
    {
        page_fill_ = fill;
        mark_layout_dirty();
    }
    bool page_fill() const { return page_fill_; }

    // 标签栏与页面之间的距离：负值 = 跟随主题 spacing（默认）。
    // 页需要贴着标签栏铺满时（例如窗口内的调试页）设 0，否则会露出一条窗口底色缝。
    void set_tab_gap(float gap)
    {
        tab_gap_ = gap;
        mark_layout_dirty();
    }
    float tab_gap() const { return tab_gap_ >= 0.0f ? tab_gap_ : ui_theme().spacing; }

    void set_context(UIContext* context) override
    {
        Widget::set_context(context);
        // 与 Panel 同规则：显式 set_padding() 过就不再用主题值覆盖。
        if (context) apply_theme_padding(context->theme().padding);
    }

    void invalidate_measure() override
    {
        // 子项无需递归：measure 缓存带布局轮次键，下一轮自然重算。
        Widget::invalidate_measure();
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        if (measure_cache_hit(constraints)) return measure_cached();

        const float pad = padding_left() + padding_right();
        const float pad_v = padding_top() + padding_bottom();
        const glm::vec2 bar = tab_bar().measure(constraints);

        glm::vec2 content{0.0f, 0.0f};
        // 无界轴保持无界（减 pad 会把它变成“有界”）。
        const glm::vec2 inner{is_bounded(constraints.x) ? std::max(0.0f, constraints.x - pad)
                                                        : constraints.x,
                              is_bounded(constraints.y) ? std::max(0.0f, constraints.y - pad_v)
                                                        : constraints.y};
        // 只测当前页：非当前页不参与尺寸协商，否则页面切回来时尺寸漂移。
        // page_fill 下当前页直接吃满内容区，不必测。
        for (std::size_t i = 1; i < child_count(); ++i) {
            if (static_cast<int>(i) - 1 != selected()) child_at(i)->mark_measured();
        }
        if (Widget* current = page(selected())) {
            if (page_fill_ && is_bounded(inner.x) && is_bounded(inner.y)) {
                content = inner;
            } else {
                const glm::vec2 cs = Widget::measure_child(*current, inner);
                content = {std::min(cs.x, is_bounded(inner.x) ? inner.x : cs.x),
                           std::min(cs.y, is_bounded(inner.y) ? inner.y : cs.y)};
            }
        }
        // 标签栏占交叉轴尺寸（侧边栏时为宽）。
        const bool side_bar = bar_position_ == BarPosition::Left || bar_position_ == BarPosition::Right;
        if (side_bar && page_fill_ && is_bounded(inner.y)) content.y = std::max(content.y, inner.y);
        if (!side_bar && page_fill_ && is_bounded(inner.x)) content.x = std::max(content.x, inner.x);

        const bool side = bar_position_ == BarPosition::Left || bar_position_ == BarPosition::Right;
        const float gap = tab_gap();
        const glm::vec2 result = side
            ? glm::vec2{bar.x + gap + content.x + pad, std::max(bar.y, content.y) + pad_v}
            : glm::vec2{std::max(bar.x, content.x) + pad, bar.y + gap + content.y + pad_v};
        store_measure(constraints, result);
        return result;
    }

    void arrange(Rect slot) override
    {
        set_geometry(slot);

        const Rect inner = layout::content_rect(*this, slot);
        const float inner_w = inner.size.x;
        const float inner_h = inner.size.y;
        const float gap = tab_gap();

        const glm::vec2 bar_size = tab_bar().measure({inner_w, inner_h});
        Rect bar_slot{};
        Rect content{};

        switch (bar_position_) {
            case BarPosition::Top: {
                const float bar_h = std::min(bar_size.y, inner_h);
                bar_slot = {inner.pos, {inner_w, bar_h}};
                content = {{inner.pos.x, inner.pos.y + bar_h + gap},
                           {inner_w, std::max(0.0f, inner_h - bar_h - gap)}};
                break;
            }
            case BarPosition::Bottom: {
                const float bar_h = std::min(bar_size.y, inner_h);
                content = {inner.pos, {inner_w, std::max(0.0f, inner_h - bar_h - gap)}};
                bar_slot = {{inner.pos.x, inner.pos.y + inner_h - bar_h}, {inner_w, bar_h}};
                break;
            }
            case BarPosition::Left: {
                const float bar_w = std::min(bar_size.x, inner_w);
                const float bar_h = std::min(bar_size.y, inner_h);
                bar_slot = {inner.pos, {bar_w, bar_h}};
                content = {{inner.pos.x + bar_w + gap, inner.pos.y},
                           {std::max(0.0f, inner_w - bar_w - gap), inner_h}};
                break;
            }
            case BarPosition::Right: {
                const float bar_w = std::min(bar_size.x, inner_w);
                const float bar_h = std::min(bar_size.y, inner_h);
                content = {inner.pos, {std::max(0.0f, inner_w - bar_w - gap), inner_h}};
                bar_slot = {{inner.pos.x + inner_w - bar_w, inner.pos.y}, {bar_w, bar_h}};
                break;
            }
        }

        tab_bar().arrange(bar_slot);

        for (std::size_t i = 1; i < child_count(); ++i) {
            auto* child = child_at(i);
            glm::vec2 size = content.size;
            if (!page_fill_) {
                const glm::vec2 measured = Widget::measure_child(*child, content.size);
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
        for (std::size_t i = 1; i < child_count(); ++i) {
            const auto* child = child_at(i);
            if (!child->visible()) continue;
            const Rect page_bounds = child->global_bounds();
            list.push_clip({page_bounds.pos.x, page_bounds.pos.y, page_bounds.size.x, page_bounds.size.y});
            child->append_to(list);
            list.pop_clip();
        }
        list.pop_clip();
    }

    bool hit_testable() const override { return false; }
    Overflow overflow() const override { return Overflow::Clip; }


private:
    LayoutNode* pages_owner(int page_index)
    {
        return child_at(static_cast<std::size_t>(page_index) + 1);
    }

    void apply_selection(int index)
    {
        for (std::size_t i = 1; i < child_count(); ++i) {
            child_at(i)->set_visible(static_cast<int>(i) - 1 == index);
        }
        mark_layout_dirty();
    }

    std::function<void(int)> on_change_;
    BarPosition bar_position_ = BarPosition::Top;
    // 负值 = 跟随主题 spacing。见 set_tab_gap()。
    float tab_gap_ = -1.0f;
    bool page_fill_ = true;
};
} // namespace xel::ui
