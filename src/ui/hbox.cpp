#include "hbox.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
Hbox::Hbox(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
}

Hbox::Hbox(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
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
        child->append_to(batch);
    }
}

Align Hbox::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

glm::vec2 Hbox::measure(glm::vec2 constraints)
{
    const float inner_w = std::max(0.0f, constraints.x - padding_ * 2.0f);
    const float inner_h = std::max(0.0f, constraints.y - padding_ * 2.0f);

    float total_w = padding_ * 2.0f;
    float max_h = 0.0f;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (i > 0) total_w += spacing_;
        const glm::vec2 cs = children_[i]->measure({inner_w, inner_h});
        total_w += cs.x;
        max_h = std::max(max_h, cs.y);
    }
    return {total_w, max_h + padding_ * 2.0f};
}

void Hbox::arrange(Rect slot)
{
    global_ = slot;
    if (children_.empty()) return;

    const float inner_w = std::max(0.0f, slot.size.x - padding_ * 2.0f);
    const float inner_h = std::max(0.0f, slot.size.y - padding_ * 2.0f);

    std::vector<glm::vec2> sizes;
    sizes.reserve(children_.size());
    float natural_w = 0.0f;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const glm::vec2 cs = children_[i]->measure({inner_w, inner_h});
        sizes.push_back(cs);
        natural_w += cs.x;
        if (i > 0) natural_w += spacing_;
    }

    float x = slot.pos.x + padding_;
    float gap = spacing_;
    const float extra = inner_w - natural_w;

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
        glm::vec2 cs = sizes[i];
        const Align a = resolve_align(*child);

        float cy;
        if (a.stretch_y) {
            cs.y = inner_h;
            cy = slot.pos.y + padding_;
        } else {
            cy = slot.pos.y + padding_ + (inner_h - cs.y) * a.y;
        }

        child->arrange({{x, cy}, {cs.x, cs.y}});
        x += cs.x + gap;
    }
}
} // namespace xel::ui
