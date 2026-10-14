#include "root.hpp"

#include "../graphics/line.hpp"

namespace xel::ui
{
UIRoot::UIRoot(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
}

void UIRoot::add(std::unique_ptr<Widget> child)
{
    children_.push_back(std::move(child));
}

void UIRoot::append_to(graphics::DrawBatch& batch) const
{
    if (is_debug()) { debug_rect(batch); }
    for (auto& child : children_) {
        child->append_to(batch);
    }
}

void UIRoot::layout()
{
    for (auto& child : children_) {
        child->layout();
    }
}
} // namespace xel::ui