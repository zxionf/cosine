#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace xel::ui
{
class Splitter : public Widget
{
public:
    enum class Orientation { Horizontal, Vertical };

    Splitter(float width, float height, Orientation orientation = Orientation::Horizontal)
        : orientation_{orientation}, preferred_width_{width}, preferred_height_{height}
    {}

    void set_panes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second)
    {
        children_.clear();
        if (first) {
            first->set_parent(this);
            first->set_context(context());
            children_.push_back(std::move(first));
        }
        if (second) {
            second->set_parent(this);
            second->set_context(context());
            children_.push_back(std::move(second));
        }
        mark_layout_dirty();
    }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }
    bool hit_testable() const override { return true; }
    bool clips_children() const override { return true; }

    void set_ratio(float ratio) { ratio_ = std::clamp(ratio, 0.1f, 0.9f); mark_layout_dirty(); }
    float ratio() const { return ratio_; }
    void set_orientation(Orientation orientation) { orientation_ = orientation; mark_layout_dirty(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return {preferred_width_, preferred_height_};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        const float handle = handle_size();
        if (children_.size() < 2) {
            if (!children_.empty()) children_[0]->arrange(slot);
            return;
        }

        if (orientation_ == Orientation::Horizontal) {
            const float usable = std::max(0.0f, slot.size.x - handle);
            const float first_w = usable * ratio_;
            children_[0]->arrange({{slot.pos.x, slot.pos.y}, {first_w, slot.size.y}});
            children_[1]->arrange({{slot.pos.x + first_w + handle, slot.pos.y},
                                   {usable - first_w, slot.size.y}});
        } else {
            const float usable = std::max(0.0f, slot.size.y - handle);
            const float first_h = usable * ratio_;
            children_[0]->arrange({{slot.pos.x, slot.pos.y}, {slot.size.x, first_h}});
            children_[1]->arrange({{slot.pos.x, slot.pos.y + first_h + handle},
                                   {slot.size.x, usable - first_h}});
        }
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const float handle = handle_size();
        const float x = orientation_ == Orientation::Horizontal
            ? global_.pos.x + (global_.size.x - handle) * ratio_
            : global_.pos.x;
        const float y = orientation_ == Orientation::Horizontal
            ? global_.pos.y
            : global_.pos.y + (global_.size.y - handle) * ratio_;
        const float w = orientation_ == Orientation::Horizontal ? handle : global_.size.x;
        const float h = orientation_ == Orientation::Horizontal ? global_.size.y : handle;

        graphics::Rectangle bar{x, y, w, h, t.panel_border};
        bar.append_to(list);

        for (const auto& child : children_) {
            if (child->visible()) {
                const auto b = child->global_bounds();
                list.push_clip({b.pos.x, b.pos.y, b.size.x, b.size.y});
                child->append_to(list);
                list.pop_clip();
            }
        }
    }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!handle_rect().contains(pos)) return false;
        dragging_ = true;
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!dragging_) return false;
        const float handle = handle_size();
        if (orientation_ == Orientation::Horizontal) {
            const float usable = std::max(1.0f, global_.size.x - handle);
            set_ratio((pos.x - global_.pos.x) / usable);
        } else {
            const float usable = std::max(1.0f, global_.size.y - handle);
            set_ratio((pos.y - global_.pos.y) / usable);
        }
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        dragging_ = false;
        return true;
    }

private:
    float handle_size() const { return 6.0f; }

    Rect handle_rect() const
    {
        const float handle = handle_size();
        if (orientation_ == Orientation::Horizontal) {
            return {
                {global_.pos.x + (global_.size.x - handle) * ratio_, global_.pos.y},
                {handle, global_.size.y}
            };
        }
        return {
            {global_.pos.x, global_.pos.y + (global_.size.y - handle) * ratio_},
            {global_.size.x, handle}
        };
    }

    Orientation orientation_ = Orientation::Horizontal;
    float ratio_ = 0.5f;
    float preferred_width_ = 400.0f;
    float preferred_height_ = 300.0f;
    bool dragging_ = false;
    std::vector<std::unique_ptr<Widget>> children_;
};
} // namespace xel::ui
