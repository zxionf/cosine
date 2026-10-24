#include "scroll_view.hpp"

#include <algorithm>

namespace xel::ui
{
void ScrollView::set_content(std::unique_ptr<Widget> c)
{
    children_.clear();
    content_ = nullptr;
    if (c) {
        c->set_parent(this);
        content_ = c.get();
        children_.push_back(std::move(c));
    }
    mark_layout_dirty();
}

glm::vec2 ScrollView::content_constraints(glm::vec2 constraints) const
{
    constexpr float kInf = 1e9f;
    switch (axis_) {
        case Axis::Vertical:   return {constraints.x, kInf};
        case Axis::Horizontal: return {kInf, constraints.y};
        case Axis::Both:       return {kInf, kInf};
    }
    return constraints;
}

glm::vec2 ScrollView::clamp_offset(glm::vec2 offset, glm::vec2 viewport) const
{
    const float max_x = (axis_ == Axis::Horizontal || axis_ == Axis::Both)
        ? std::max(0.0f, content_size_.x - viewport.x) : 0.0f;
    const float max_y = (axis_ == Axis::Vertical || axis_ == Axis::Both)
        ? std::max(0.0f, content_size_.y - viewport.y) : 0.0f;

    return {
        std::clamp(offset.x, 0.0f, max_x),
        std::clamp(offset.y, 0.0f, max_y)
    };
}

void ScrollView::set_scroll_offset(glm::vec2 offset)
{
    offset_ = clamp_offset(offset, global_.size);
    mark_layout_dirty();
}

glm::vec2 ScrollView::measure(glm::vec2 constraints)
{
    if (!content_) {
        content_size_ = {};
        return {0.0f, 0.0f};
    }

    content_size_ = content_->measure(content_constraints(constraints));

    glm::vec2 result = constraints;
    if (axis_ == Axis::Vertical || axis_ == Axis::Both) {
        result.y = std::min(content_size_.y, constraints.y);
    }
    if (axis_ == Axis::Horizontal || axis_ == Axis::Both) {
        result.x = std::min(content_size_.x, constraints.x);
    }
    if (axis_ == Axis::Vertical) {
        result.x = std::min(content_size_.x, constraints.x);
    }
    if (axis_ == Axis::Horizontal) {
        result.y = std::min(content_size_.y, constraints.y);
    }
    return result;
}

void ScrollView::arrange(Rect slot)
{
    global_ = slot;
    if (!content_) return;

    content_size_ = content_->measure(content_constraints(slot.size));
    offset_ = clamp_offset(offset_, slot.size);

    content_->arrange({
        {slot.pos.x - offset_.x, slot.pos.y - offset_.y},
        content_size_
    });
}

bool ScrollView::on_scroll(glm::vec2 pos, glm::vec2 delta)
{
    if (!global_bounds().contains(pos)) return false;

    if (axis_ == Axis::Vertical || axis_ == Axis::Both) {
        offset_.y -= delta.y * 40.0f;
    }
    if (axis_ == Axis::Horizontal || axis_ == Axis::Both) {
        offset_.x -= delta.x * 40.0f;
    }
    offset_ = clamp_offset(offset_, global_.size);
    mark_layout_dirty();
    return true;
}

void ScrollView::append_to(graphics::DrawList& list) const
{
    if (!content_) return;
    list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
    content_->append_to(list);
    list.pop_clip();
}
} // namespace xel::ui
