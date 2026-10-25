#include "root.hpp"

namespace xel::ui
{
UIRoot::UIRoot(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
    set_context(&context_storage_);
}

void UIRoot::add(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    child->set_context(context());
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void UIRoot::add_overlay(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    child->set_context(context());
    overlays_.push_back(std::move(child));
    mark_layout_dirty();
}

void UIRoot::remove_overlay(Widget* overlay)
{
    if (!overlay) return;
    for (auto it = overlays_.begin(); it != overlays_.end(); ++it) {
        if (it->get() == overlay) {
            trash_.push_back(std::move(*it));
            overlays_.erase(it);
            mark_layout_dirty();
            return;
        }
    }
}

void UIRoot::purge_trash()
{
    trash_.clear();
}

void UIRoot::set_theme(const Theme& value)
{
    context_storage_.theme = value;
    set_context(&context_storage_);
    mark_layout_dirty();
}

void UIRoot::append_to(graphics::DrawList& list) const
{
    for (const auto& child : children_) {
        if (child->visible()) child->append_to(list);
    }
    for (const auto& overlay : overlays_) {
        if (overlay->visible()) overlay->append_to(list);
    }
    if (is_debug()) append_debug_tree(list);
}

void UIRoot::layout_child(Widget& child)
{
    const glm::vec2 child_size = child.clamp_size(child.measure(local_.size));
    child.arrange({
        {local_.pos.x + child.x(), local_.pos.y + child.y()},
        child_size
    });
}

void UIRoot::layout()
{
    purge_trash();
    for (auto& child : children_) layout_child(*child);
    for (auto& overlay : overlays_) layout_child(*overlay);
}

glm::vec2 UIRoot::measure(glm::vec2 constraints)
{
    (void)constraints;
    return local_.size;
}

void UIRoot::arrange(Rect slot)
{
    global_ = slot;
    set_pos(slot.pos);
    set_size(slot.size);
    layout();
}
} // namespace xel::ui
