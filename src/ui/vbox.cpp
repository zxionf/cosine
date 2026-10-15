#include "vbox.hpp"

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
    children_.push_back(std::move(child));
}

void Vbox::layout()
{
    // float cur_y = y() + padding_;
    // for (auto& child : children_) {
    //     child->set_pos(x() + padding_, cur_y);
    //     cur_y += child->height() + spacing_;
    // }
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
    float total_h = 0.0f;
    float max_w = 0.0f;
    bool first = true;
    for (auto& c : children_) {
        if (first) total_h += spacing_;
        first = false;
        glm::vec2 cs = c->measure(constraints);
        total_h += cs.y;
        max_w = std::max(max_w, cs.x);
    }
    return {max_w, total_h};
}

void Vbox::arrange(Rect slot)
{
    global_ = slot;
    float y = slot.pos.y;
    for (auto& c : children_) {
        glm::vec2 cs = c->measure(slot.size);
        Align a = resolve_align(*c);
        float cx = slot.pos.x + (slot.size.x - cs.x) * a.x;

        Rect child_slot{{cx, y}, {cs.x, cs.y}};
        c->arrange(child_slot);

        y += cs.y + spacing_;
    }
}
} // namespace xel::ui