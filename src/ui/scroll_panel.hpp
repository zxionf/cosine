#pragma once

#include "panel.hpp"
#include "scroll_view.hpp"
#include "vbox.hpp"

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
    void add(std::unique_ptr<Widget> child) override
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
        const float pad_h = padding_left() + padding_right();
        const float pad_v = padding_top() + padding_bottom();
        const float available_w = constraints.x > 0.0f && constraints.x < 1.0e8f
            ? std::max(0.0f, constraints.x - pad_h)
            : 1.0e8f;
        const glm::vec2 inner = content_->measure({available_w, 1.0e9f});
        return {inner.x + pad_h, inner.y + pad_v};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        if (!scroll_) return;

        const float pad_l = padding_left();
        const float pad_t = padding_top();
        const Rect inner{
            {slot.pos.x + pad_l, slot.pos.y + pad_t},
            {std::max(0.0f, slot.size.x - pad_l - padding_right()),
             std::max(0.0f, slot.size.y - pad_t - padding_bottom())}
        };
        scroll_->arrange(inner);
    }

    void invalidate_measure() override
    {
        measure_valid_ = false;
        if (content_) content_->invalidate_measure();
    }

private:
    void init()
    {
        auto box = std::make_unique<Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        box->set_cross_align(Align::top_left());
        box->set_padding(0.0f);
        box->set_spacing(ui_theme().spacing);
        content_ = box.get();

        auto scroll = std::make_unique<ScrollView>();
        scroll_ = scroll.get();
        scroll->set_content(std::move(box));
        Panel::add(std::move(scroll));

        content_->set_context(context());
    }

    ScrollView* scroll_ = nullptr;
    Vbox* content_ = nullptr;
    bool measure_valid_ = false;
};
} // namespace xel::ui
