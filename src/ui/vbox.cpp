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
    float cur_y = y() + padding_;
    for (auto& child : children_) {
        child->set_pos(x() + padding_, cur_y);
        cur_y += child->height() + spacing_;
    }
}

void Vbox::append_to(graphics::DrawBatch& batch) const
{
    if (is_debug()) { debug_rect(batch); }
    for (auto& child : children_) {
        child->append_to(batch);
    }
}
} // namespace xel::ui