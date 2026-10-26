#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/box.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <memory>
#include <utility>
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

    void set_ratio(float ratio)
    {
        const auto limits = ratio_limits();
        ratio_ = std::clamp(ratio, limits.first, limits.second);
        mark_layout_dirty();
    }
    float ratio() const { return ratio_; }
    // 当前尺寸下可用的比例区间（由两个子项的 min_size 推导）。
    std::pair<float, float> ratio_limits() const
    {
        constexpr float kFallbackMin = 0.1f;
        constexpr float kFallbackMax = 0.9f;
        const float handle = handle_size();
        const bool horizontal = orientation_ == Orientation::Horizontal;
        const float extent = horizontal ? global_.size.x : global_.size.y;
        const float usable = extent - handle;
        if (usable <= 0.0f || children_.size() < 2) return {kFallbackMin, kFallbackMax};

        const glm::vec2 first_min = children_[0]->min_size();
        const glm::vec2 second_min = children_[1]->min_size();
        const float first_need = horizontal ? first_min.x : first_min.y;
        const float second_need = horizontal ? second_min.x : second_min.y;

        const float lower = std::max(kFallbackMin, std::min(kFallbackMax, first_need / usable));
        const float upper = std::min(kFallbackMax, std::max(lower, 1.0f - second_need / usable));
        return {lower, upper};
    }
    void set_orientation(Orientation orientation) { orientation_ = orientation; mark_layout_dirty(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return {preferred_width_, preferred_height_};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        const auto limits = ratio_limits();
        ratio_ = std::clamp(ratio_, limits.first, limits.second);
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

        // 手柄：颜色沿用 panel_border，圆角随主题（classic 为直角）。
        const BoxStyle bar_style = visual_style(StyleRole::Control,
                                                is_hovered() ? ControlState::Hovered : ControlState::Normal);
        glm::vec4 bar_color = t.panel_border;
        if (dragging_) {
            bar_color = with_alpha(t.accent, 0.28f);
        } else if (is_hovered()) {
            bar_color = bar_style.fill;
        }
        const float bar_radius = corner_radius(t.radius_small);
        graphics::append_rounded_rect(list, {x, y, w, h}, bar_radius, bar_color);
        // 拖动反馈：描边强调色细环。
        if (dragging_) {
            graphics::append_rounded_ring(list, {x, y, w, h}, bar_radius, 1.0f, t.accent);
        }

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
