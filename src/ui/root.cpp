#include "root.hpp"

#include "../graphics/line.hpp"
#include "vbox.hpp"
#include "color_rect.hpp"

namespace xel::ui
{
UIRoot::UIRoot(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
    left_column_ = std::make_unique<Vbox>(0, 0, 360, global_.size.y);
    left_column_->debug();
    left_column_->add(std::make_unique<ColorRect>(0, 0, 360, 50, glm::vec4(0.5, 0.5, 0.5, 1)));
    left_column_->add(std::make_unique<ColorRect>(0, 0, 360, 50, glm::vec4(0.5, 0.5, 0.5, 1)));
    left_column_->add(std::make_unique<ColorRect>(0, 0, 360, 50, glm::vec4(0.5, 0.5, 0.5, 1)));
    add(std::move(left_column_));
}

void UIRoot::add(std::unique_ptr<Widget> child)
{
    children_.push_back(std::move(child));
}

void UIRoot::append_to(graphics::DrawBatch& batch) const
{
    if (is_debug()) { debug_rect(batch); }
    // left_column_->append_to(batch);
    for (auto& child : children_) {
        child->append_to(batch);
    }
}

void UIRoot::layout()
{
    // left_column_->set_pos(x(), y());
    // left_column_->set_size(360, height());
    for (auto& child : children_) {
        child->set_pos(x()+child->x(), y()+child->y());
        child->layout();
    }
}

glm::vec2 UIRoot::measure(glm::vec2 constraints)
{
    for (auto& child : children_) {
        child->measure(constraints);
    }
    return local_bounds().size;
}

void UIRoot::arrange(Rect slot)
{
    for (auto& child : children_) {
        child->arrange(slot);
    }
}
} // namespace xel::ui