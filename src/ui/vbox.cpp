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
        if (debug_) {
            auto [x, y, w, h] = local_bounds();
            graphics::Line top_l    {x,     y,      x + w,  y,      1.0f, {0, 1, 0, 1}};
            graphics::Line right_l  {x + w, y,      x + w,  y + h,  1.0f, {0, 1, 0, 1}};
            graphics::Line bottom_l {x,     y + h,  x + w,  y + h,  1.0f, {0, 1, 0, 1}};
            graphics::Line left_l   {x,     y,      x,      y + h,  1.0f, {0, 1, 0, 1}};
            top_l.append_to(batch);
            right_l.append_to(batch);
            bottom_l.append_to(batch);
            left_l.append_to(batch);
        }
        for (auto& child : children_) {
            child->append_to(batch);
        }
    }
} // namespace xel::ui