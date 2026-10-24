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
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void Hbox::layout()
{
    arrange(local_bounds());
}

void Hbox::append_to(graphics::DrawBatch& batch) const
{
    if (is_debug()) { debug_rect(batch); }
    for (const auto& child : children_) {
        if (child->visible()) child->append_to(batch);
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

    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (i > 0) total_w += spacing_;

        const auto& child = children_[i];
        const float ml = child->margin_left();
        const float mr = child->margin_right();
        const float mt = child->margin_top();
        const float mb = child->margin_bottom();

        glm::vec2 cs = child->measure({inner_w, std::max(0.0f, inner_h - mt - mb)});
        cs = child->clamp_size(cs);

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
        glm::vec2 size;
        float ml, mt, mr, mb;
    };

    std::vector<Item> items;
    items.reserve(children_.size());
    float natural_w = 0.0f;

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto& child = children_[i];
        const float ml = child->margin_left();
        const float mt = child->margin_top();
        const float mr = child->margin_right();
        const float mb = child->margin_bottom();

        glm::vec2 cs = child->measure({inner_w, std::max(0.0f, inner_h - mt - mb)});
        cs = child->clamp_size(cs);

        items.push_back({cs, ml, mt, mr, mb});
        natural_w += ml + cs.x + mr;
        if (i > 0) natural_w += spacing_;
    }

    float extra = inner_w - natural_w;
    if (extra > 0.0f) {
        float grow_sum = 0.0f;
        for (const auto& child : children_) grow_sum += child->flex_grow();

        if (grow_sum > 0.0f) {
            for (std::size_t i = 0; i < children_.size(); ++i) {
                const float grow = children_[i]->flex_grow();
                if (grow <= 0.0f) continue;
                items[i].size = children_[i]->clamp_size({
                    items[i].size.x + extra * grow / grow_sum,
                    items[i].size.y
                });
            }

            natural_w = 0.0f;
            for (std::size_t i = 0; i < items.size(); ++i) {
                natural_w += items[i].ml + items[i].size.x + items[i].mr;
                if (i > 0) natural_w += spacing_;
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
            if (children_.size() > 1 && extra > 0.0f) {
                gap += extra / static_cast<float>(children_.size() - 1);
            }
            break;
    }

    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        auto& item = items[i];
        const Align a = resolve_align(*child);
        const float available_h = std::max(0.0f, inner_h - item.mt - item.mb);

        float cy;
        if (a.stretch_y) {
            item.size.y = available_h;
            cy = inner_y + item.mt;
        } else {
            cy = inner_y + item.mt + (available_h - item.size.y) * a.y;
        }

        child->arrange({{x + item.ml, cy}, item.size});
        x += item.ml + item.size.x + item.mr + gap;
    }
}
} // namespace xel::ui
