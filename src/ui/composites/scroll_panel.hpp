#pragma once

#include "ui/containers/panel.hpp"
#include "ui/containers/scroll_view.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/geom/layout_metrics.hpp"
#include "ui/layout/layout_utils.hpp"

#include <algorithm>
#include <memory>

namespace xel::ui
{
// 可滚动的面板：面板内是 ScrollView，`add()` 的内容进入滚动区。
// 用于演示/设置页等"内容可能超出可视高度"的场景。
class ScrollPanel : public Panel
{
public:
    ScrollPanel(float x, float y, float width, float height)
        : Panel(x, y, width, height)
    {
        init();
    }

    explicit ScrollPanel(glm::vec2 size) : Panel(size) { init(); }

    ScrollView* scroll_view() { return scroll_; }
    const ScrollView* scroll_view() const { return scroll_; }
    Vbox* content() { return content_; }
    const Vbox* content() const { return content_; }

    // 内容进入滚动区；需要固定表头时直接使用 content()->add() 之外的 API。
    void add(std::unique_ptr<Node> child) override
    {
        content_->add(std::move(child));
    }

    void set_context(UIContext* context) override
    {
        Panel::set_context(context);
        if (scroll_) scroll_->set_context(context);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        if (measure_cache_hit(constraints)) return measure_cached();
        const float pad_h = padding_left() + padding_right();
        const float pad_v = padding_top() + padding_bottom();
        // 宽度有界时按可用宽测量（内容可换行），否则交给内容自然宽。
        const float available_w = is_bounded(constraints.x)
            ? std::max(0.0f, constraints.x - pad_h)
            : kUnbounded;
        // 高度总是无界：内容高度由内容决定，视口高度由父级安排（ScrollPanel 必须放在有界父容器里）。
        const glm::vec2 inner = Widget::measure_child(*content_, {available_w, kUnbounded});
        const glm::vec2 result{inner.x + pad_h, inner.y + pad_v};
        store_measure(constraints, result);
        return result;
    }

    void arrange(Rect slot) override
    {
        set_geometry(slot);
        if (!scroll_) return;
        scroll_->arrange(layout::content_rect(*this, slot));
    }

    void invalidate_measure() override
    {
        // 内容无需递归：measure 缓存带布局轮次键。
        Widget::invalidate_measure();
    }

private:
    void init()
    {
        auto box = std::make_unique<Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        box->set_cross_align(Align::top_left());
        box->set_padding(0.0f);
        box->apply_theme_spacing(ui_theme().spacing);
        content_ = box.get();

        auto scroll = std::make_unique<ScrollView>();
        scroll_ = scroll.get();
        scroll->set_content(std::move(box));
        Panel::add(std::move(scroll));

        content_->set_context(context());
    }

    ScrollView* scroll_ = nullptr;
    Vbox* content_ = nullptr;
};
} // namespace xel::ui
