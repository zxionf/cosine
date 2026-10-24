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

glm::vec2 ScrollView::measure(glm::vec2 constraints)
{
    if (!content_) {
        content_size_ = {};
        return {0.0f, 0.0f};
    }

    const glm::vec2 inf = {constraints.x, 1e9f};
    content_size_ = content_->measure(inf);

    return {
        std::min(content_size_.x, constraints.x),
        std::min(content_size_.y, constraints.y)
    };
}

void ScrollView::arrange(Rect slot)
{
    global_ = slot;
    if (!content_) return;

    content_size_ = content_->measure({slot.size.x, 1e9f});

    const float max_offset = std::max(0.0f, content_size_.y - slot.size.y);
    offset_ = std::clamp(offset_, 0.0f, max_offset);

    content_->arrange({
        {slot.pos.x, slot.pos.y - offset_},
        {content_size_.x, content_size_.y}
    });
}

bool ScrollView::on_scroll(glm::vec2 pos, float delta)
{
    if (!global_bounds().contains(pos)) return false;

    const float max_offset = std::max(0.0f, content_size_.y - global_.size.y);
    // GLFW: 向上滚 yoffset 为正；向上滚应让内容下移，即 offset 减小。
    offset_ -= delta * 40.0f;
    offset_ = std::clamp(offset_, 0.0f, max_offset);
    return true;
}

void ScrollView::append_to(graphics::DrawBatch& batch) const
{
    if (!content_) return;
    content_->append_to(batch);
}
} // namespace xel::ui
