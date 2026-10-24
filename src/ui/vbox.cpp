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
    if (children_.empty()) return;

    const float inner_w = std::max(0.0f, slot.size.x - padding_ * 2.0f);
    const float inner_h = std::max(0.0f, slot.size.y - padding_ * 2.0f);

    std::vector<glm::vec2> sizes;
    sizes.reserve(children_.size());
    float natural_h = 0.0f;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const glm::vec2 cs = children_[i]->measure({inner_w, inner_h});
        sizes.push_back(cs);
        natural_h += cs.y;
        if (i > 0) natural_h += spacing_;
    }

    float y = slot.pos.y + padding_;
    float gap = spacing_;
    const float extra = inner_h - natural_h;

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
        glm::vec2 cs = sizes[i];
        const Align a = resolve_align(*child);
        float cx;
        if (a.stretch_x) {
            cs.x = inner_w;
            cx = slot.pos.x + padding_;
        } else {
            cx = slot.pos.x + padding_ + (inner_w - cs.x) * a.x;
        }

        child->arrange({{cx, y}, {cs.x, cs.y}});
        y += cs.y + gap;
    }
}
} // namespace xel::ui
