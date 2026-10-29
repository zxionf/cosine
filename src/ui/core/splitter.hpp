#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "graphics/box.hpp"
#include "graphics/rectangle.hpp"

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
        if (first) adopt(std::move(first));
        if (second) adopt(std::move(second));
        mark_layout_dirty();
    }

    bool hit_testable() const override { return true; }
    bool clips_children() const override { return true; }

    void set_ratio(float ratio)
    {
        const auto limits = ratio_limits();
        ratio_ = std::clamp(ratio, limits.first, limits.second);
        mark_layout_dirty();
    }

    // 固定一侧的像素宽度（另一侧弹性）。传入非负值启用，-1 回到比例模式。
    // 两者互斥：设一个会清掉另一个。
    void set_fixed_first(float pixels)
    {
        fixed_first_ = pixels >= 0.0f ? pixels : -1.0f;
        fixed_second_ = -1.0f;
        mark_layout_dirty();
    }
    void set_fixed_second(float pixels)
    {
        fixed_second_ = pixels >= 0.0f ? pixels : -1.0f;
        fixed_first_ = -1.0f;
        mark_layout_dirty();
    }
    bool fixed_mode() const { return fixed_first_ >= 0.0f || fixed_second_ >= 0.0f; }
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
            const float first_w = resolved_first();
            children_[0]->arrange({{slot.pos.x, slot.pos.y}, {first_w, slot.size.y}});
            children_[1]->arrange({{slot.pos.x + first_w + handle, slot.pos.y},
                                   {usable - first_w, slot.size.y}});
        } else {
            const float usable = std::max(0.0f, slot.size.y - handle);
            const float first_h = resolved_first();
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
            ? global_.pos.x + resolved_first()
            : global_.pos.x;
        const float y = orientation_ == Orientation::Horizontal
            ? global_.pos.y
            : global_.pos.y + resolved_first();
        const float w = orientation_ == Orientation::Horizontal ? handle : global_.size.x;
        const float h = orientation_ == Orientation::Horizontal ? global_.size.y : handle;

        // 手柄：常态用主题的 splitter 色（比面板亮一档），hover 提亮，拖动变强调色。
        const BoxStyle bar_style = visual_style(StyleRole::Control,
                                                is_hovered() ? ControlState::Hovered : ControlState::Normal);
        glm::vec4 bar_color = t.splitter;
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
            apply_drag(pos.x - global_.pos.x, usable);
        } else {
            const float usable = std::max(1.0f, global_.size.y - handle);
            apply_drag(pos.y - global_.pos.y, usable);
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

    // 第一块的像素尺寸：固定模式优先，否则按比例。
    float resolved_first() const
    {
        const float handle = handle_size();
        const bool horizontal = orientation_ == Orientation::Horizontal;
        const float extent = horizontal ? global_.size.x : global_.size.y;
        const float usable = std::max(0.0f, extent - handle);
        if (fixed_first_ >= 0.0f) return std::min(fixed_first_, usable);
        if (fixed_second_ >= 0.0f) return std::max(0.0f, usable - fixed_second_);
        return usable * ratio_;
    }

    // 拖动：固定模式改像素，比例模式改 ratio。
    void apply_drag(float first_px, float usable)
    {
        const float clamped = std::clamp(first_px, 0.0f, usable);
        if (fixed_first_ >= 0.0f) {
            fixed_first_ = clamped;
        } else if (fixed_second_ >= 0.0f) {
            fixed_second_ = usable - clamped;
        } else {
            set_ratio(clamped / usable);
        }
        mark_layout_dirty();
    }

    Rect handle_rect() const
    {
        const float handle = handle_size();
        if (orientation_ == Orientation::Horizontal) {
            return {
                {global_.pos.x + resolved_first(), global_.pos.y},
                {handle, global_.size.y}
            };
        }
        return {
            {global_.pos.x, global_.pos.y + resolved_first()},
            {global_.size.x, handle}
        };
    }

    Orientation orientation_ = Orientation::Horizontal;
    float ratio_ = 0.5f;
    float fixed_first_ = -1.0f;
    float fixed_second_ = -1.0f;
    float preferred_width_ = 400.0f;
    float preferred_height_ = 300.0f;
    bool dragging_ = false;
};
} // namespace xel::ui
