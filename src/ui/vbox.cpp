#include "vbox.hpp"

namespace xel::ui
{
    Vbox::Vbox(float x, float y, float width, float height)
    : x_{x}, y_{y}, width_{width}, height_{height}
    {}

    Vbox::~Vbox()
    {}

    void Vbox::add(std::unique_ptr<Widget> child)
    {
        children_.push_back(std::move(child));
    }

    void Vbox::layout()
    {
        float cur_y = y_ + padding_;
        for (auto& child : children_) {
            child->set_position(x_ + padding_, cur_y);
            cur_y += child->height() + spacing_;
        }
    }

    void Vbox::append_to(graphics::DrawBatch& batch) const
    {
        if (debug_) {
            graphics::Line top_l{x_, y_, x_ + width_, y_, 1.0f, {0, 1, 0, 1}};
            graphics::Line right_l{x_ + width_, y_, x_ + width_, y_ + height_, 1.0f, {0, 1, 0, 1}};
            graphics::Line bottom_l{x_, y_ + height_, x_ + width_, y_ + height_, 1.0f, {0, 1, 0, 1}};
            graphics::Line left_l{x_, y_, x_, y_ + height_, 1.0f, {0, 1, 0, 1}};
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