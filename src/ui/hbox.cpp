#include "hbox.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
Hbox::Hbox(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
    set_padding(4.0f);
}

Hbox::Hbox(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
    set_padding(4.0f);
}

Hbox::~Hbox() = default;

void Hbox::add(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    child->set_context(context());
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void Hbox::layout()
{
    arrange(local_bounds());
}

void Hbox::append_to(graphics::DrawList& list) const
{
    if (is_debug()) { debug_rect(list); }
    // z-index 控制同层绘制顺序（大的在上）；没有非零 z 时保持加入顺序。
    for (Widget* child : paint_order()) {
        if (child->visible()) child->append_to(list);
    }
}

Align Hbox::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

glm::vec2 Hbox::measure(glm::vec2 constraints)
{
    if (measure_valid_ && constraints == cached_constraints_) {
        return cached_measure_;
    }

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = std::max(0.0f, constraints.x - pad_l - pad_r);
    const float inner_h = std::max(0.0f, constraints.y - pad_t - pad_b);

    float total_w = pad_l + pad_r;
    float max_h = 0.0f;
    std::size_t placed = 0;

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto& child = children_[i];
        // 绝对定位子项不参与流式尺寸。
        if (child->absolute()) continue;
        if (placed > 0) total_w += spacing_;
        ++placed;
        const float ml = child->margin_left();
        const float mr = child->margin_right();
        const float mt = child->margin_top();
        const float mb = child->margin_bottom();

        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, std::max(0.0f, inner_h - mt - mb)});

        total_w += ml + cs.x + mr;
        max_h = std::max(max_h, mt + cs.y + mb);
    }

    cached_measure_ = clamp_size({total_w, max_h + pad_t + pad_b});
    cached_constraints_ = constraints;
    measure_valid_ = true;
    return cached_measure_;
}

void Hbox::arrange(Rect slot)
{
    global_ = slot;
    if (children_.empty()) return;

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();

    const float inner_x = slot.pos.x + pad_l;
    const float inner_y = slot.pos.y + pad_t;
    const float inner_w = std::max(0.0f, slot.size.x - pad_l - pad_r);
    const float inner_h = std::max(0.0f, slot.size.y - pad_t - pad_b);

    struct Item
    {
        Widget* child;
        glm::vec2 size;
        float ml, mt, mr, mb;
    };

    std::vector<Item> items;
    items.reserve(children_.size());
    float natural_w = 0.0f;

    // 流式子项：绝对定位的子项不占位、不参与 spacing 与 flex 分配。
    for (const auto& child : children_) {
        if (child->absolute()) continue;
        const float ml = child->margin_left();
        const float mt = child->margin_top();
        const float mr = child->margin_right();
        const float mb = child->margin_bottom();

        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, std::max(0.0f, inner_h - mt - mb)});

        if (!items.empty()) natural_w += spacing_;
        items.push_back({child.get(), cs, ml, mt, mr, mb});
        natural_w += ml + cs.x + mr;
    }

    float extra = inner_w - natural_w;
    if (extra > 0.0f) {
        float grow_sum = 0.0f;
        for (const auto& item : items) grow_sum += item.child->flex_grow();

        if (grow_sum > 0.0f) {
            for (auto& item : items) {
                const float grow = item.child->flex_grow();
                if (grow <= 0.0f) continue;
                item.size = item.child->clamp_size({
                    item.size.x + extra * grow / grow_sum,
                    item.size.y
                });
            }

            natural_w = 0.0f;
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) natural_w += spacing_;
                natural_w += items[i].ml + items[i].size.x + items[i].mr;
            }
            extra = inner_w - natural_w;
        }
    }

    float x = inner_x;
    float gap = spacing_;

    switch (main_align_) {
        case MainAlign::Start:
            break;
        case MainAlign::Center:
            x += std::max(0.0f, extra) * 0.5f;
            break;
        case MainAlign::End:
            x += std::max(0.0f, extra);
            break;
        case MainAlign::SpaceBetween:
            if (items.size() > 1 && extra > 0.0f) {
                gap += extra / static_cast<float>(items.size() - 1);
            }
            break;
    }

    for (auto& item : items) {
        Widget& child = *item.child;
        const Align a = resolve_align(child);
        const float available_h = std::max(0.0f, inner_h - item.mt - item.mb);
        // 百分比尺寸优先：声明了高百分比的子项不再被 stretch 拉满。
        const bool pct_y = child.size_percent().y > 0.0f;

        float cy;
        if (a.stretch_y && !pct_y) {
            item.size.y = available_h;
            cy = inner_y + item.mt;
        } else {
            cy = inner_y + item.mt + (available_h - item.size.y) * a.y;
        }

        child.arrange({{x + item.ml, cy}, item.size});
        x += item.ml + item.size.x + item.mr + gap;
    }

    // 绝对定位子项：按自身坐标相对内容区放置，尺寸走同一套测量规则。
    for (const auto& child : children_) {
        if (!child->absolute()) continue;
        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, inner_h});
        child->arrange({{inner_x + child->x(), inner_y + child->y()}, cs});
    }
}
} // namespace xel::ui
