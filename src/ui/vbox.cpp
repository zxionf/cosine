#include "vbox.hpp"

#include <algorithm>

namespace xel::ui
{
Vbox::Vbox(float x, float y, float w, float height)
{
    set_pos(x, y);
    set_size(w, height);
}

Vbox::Vbox(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
}

Vbox::~Vbox()
{}

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

void Vbox::append_to(graphics::DrawBatch& batch) const
{
    if (is_debug()) { debug_rect(batch); }
    for (auto& child : children_) {
        child->append_to(batch);
    }
}

Align Vbox::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

glm::vec2 Vbox::measure(glm::vec2 constraints)
{
    const float inner_w = std::max(0.0f, constraints.x - padding_ * 2.0f);
    const float inner_h = std::max(0.0f, constraints.y - padding_ * 2.0f);

    float total_h = padding_ * 2.0f;
    float max_w = 0.0f;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (i > 0) total_h += spacing_;
        const glm::vec2 cs = children_[i]->measure({inner_w, inner_h});
        total_h += cs.y;
        max_w = std::max(max_w, cs.x);
    }
    return {max_w + padding_ * 2.0f, total_h};
}

void Vbox::arrange(Rect slot)
{
    global_ = slot;
    const float inner_w = std::max(0.0f, slot.size.x - padding_ * 2.0f);
    float y = slot.pos.y + padding_;

    for (auto& c : children_) {
        glm::vec2 cs = c->measure({inner_w, slot.size.y - padding_ * 2.0f});
        const Align a = resolve_align(*c);
        float cx;
        if (a.stretch_x) {
            cs.x = inner_w;
            cx = slot.pos.x + padding_;
        } else {
            cx = slot.pos.x + padding_ + (inner_w - cs.x) * a.x;
        }

        Rect child_slot{{cx, y}, {cs.x, cs.y}};
        c->arrange(child_slot);

        y += cs.y + spacing_;
    }
}
} // namespace xel::ui
