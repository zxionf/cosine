#include "vbox.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
Vbox::Vbox(float x, float y, float w, float height)
{
    set_pos(x, y);
    set_size(w, height);
    set_padding(4.0f);
}

Vbox::Vbox(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
    set_padding(4.0f);
}

Vbox::~Vbox() = default;

void Vbox::add(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void Vbox::layout()
{
    arrange(local_bounds());
}

void Vbox::append_to(graphics::DrawList& list) const
{
    if (is_debug()) { debug_rect(list); }
    for (const auto& child : children_) {
        if (child->visible()) child->append_to(list);
    }
}

Align Vbox::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

glm::vec2 Vbox::measure(glm::vec2 constraints)
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

    float total_h = pad_t + pad_b;
    float max_w = 0.0f;

    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (i > 0) total_h += spacing_;

        const auto& child = children_[i];
        const float ml = child->margin_left();
        const float mr = child->margin_right();
        const float mt = child->margin_top();
        const float mb = child->margin_bottom();

        glm::vec2 cs = child->measure({std::max(0.0f, inner_w - ml - mr), inner_h});
        cs = child->clamp_size(cs);

        total_h += mt + cs.y + mb;
        max_w = std::max(max_w, ml + cs.x + mr);
    }

    cached_measure_ = clamp_size({max_w + pad_l + pad_r, total_h});
    cached_constraints_ = constraints;
    measure_valid_ = true;
    return cached_measure_;
}

void Vbox::arrange(Rect slot)
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
        glm::vec2 size;
        float ml, mt, mr, mb;
    };

    std::vector<Item> items;
    items.reserve(children_.size());
    float natural_h = 0.0f;

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto& child = children_[i];
        const float ml = child->margin_left();
        const float mt = child->margin_top();
        const float mr = child->margin_right();
        const float mb = child->margin_bottom();

        glm::vec2 cs = child->measure({std::max(0.0f, inner_w - ml - mr), inner_h});
        cs = child->clamp_size(cs);

        items.push_back({cs, ml, mt, mr, mb});
        natural_h += mt + cs.y + mb;
        if (i > 0) natural_h += spacing_;
    }

    float extra = inner_h - natural_h;
    if (extra > 0.0f) {
        float grow_sum = 0.0f;
        for (const auto& child : children_) grow_sum += child->flex_grow();

        if (grow_sum > 0.0f) {
            for (std::size_t i = 0; i < children_.size(); ++i) {
                const float grow = children_[i]->flex_grow();
                if (grow <= 0.0f) continue;
                items[i].size = children_[i]->clamp_size({
                    items[i].size.x,
                    items[i].size.y + extra * grow / grow_sum
                });
            }

            natural_h = 0.0f;
            for (std::size_t i = 0; i < items.size(); ++i) {
                natural_h += items[i].mt + items[i].size.y + items[i].mb;
                if (i > 0) natural_h += spacing_;
            }
            extra = inner_h - natural_h;
        }
    }

    float y = inner_y;
    float gap = spacing_;

    switch (main_align_) {
        case MainAlign::Start:
            break;
        case MainAlign::Center:
            y += std::max(0.0f, extra) * 0.5f;
            break;
        case MainAlign::End:
            y += std::max(0.0f, extra);
            break;
        case MainAlign::SpaceBetween:
            if (children_.size() > 1 && extra > 0.0f) {
                gap += extra / static_cast<float>(children_.size() - 1);
            }
            break;
    }

    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        auto& item = items[i];
        const Align a = resolve_align(*child);
        const float available_w = std::max(0.0f, inner_w - item.ml - item.mr);

        float cx;
        if (a.stretch_x) {
            item.size.x = available_w;
            cx = inner_x + item.ml;
        } else {
            cx = inner_x + item.ml + (available_w - item.size.x) * a.x;
        }

        child->arrange({{cx, y + item.mt}, item.size});
        y += item.mt + item.size.y + item.mb + gap;
    }
}
} // namespace xel::ui
