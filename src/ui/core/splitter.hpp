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
    WidgetRole semantics_role() const override { return WidgetRole::Splitter; }
    enum class Orientation { Horizontal, Vertical };

    Splitter(float width, float height, Orientation orientation = Orientation::Horizontal)
        : orientation_{orientation}, preferred_width_{width}, preferred_height_{height}
    {}

    void set_panes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second)
    {
        clear_children();
        if (first) adopt(std::move(first));
        if (second) adopt(std::move(second));
        mark_layout_dirty();
    }

    bool hit_testable() const override { return true; }
    Overflow overflow() const override { return Overflow::Clip; }

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
        if (usable <= 0.0f || child_count() < 2) return {kFallbackMin, kFallbackMax};

        const glm::vec2 first_min = child_at(0)->min_size();
        const glm::vec2 second_min = child_at(1)->min_size();
        const float first_need = horizontal ? first_min.x : first_min.y;
        const float second_need = horizontal ? second_min.x : second_min.y;

        const float lower = std::max(kFallbackMin, std::min(kFallbackMax, first_need / usable));
        const float upper = std::min(kFallbackMax, std::max(lower, 1.0f - second_need / usable));
        return {lower, upper};
    }
    void set_orientation(Orientation orientation) { orientation_ = orientation; mark_layout_dirty(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        // 自然尺寸 = 两个 pane 的自然主轴尺寸 + handle（交叉轴取最大）。
        // 它只是给父级参考；实际尺寸由 arrange 的 slot 决定。
        const float handle = handle_size();
        const bool horizontal = orientation_ == Orientation::Horizontal;
        float first = 0.0f;
        float second = 0.0f;
        float cross = 0.0f;
        if (child_count() >= 1) {
            const glm::vec2 cs = Widget::measure_child(*child_at(0), constraints);
            first = horizontal ? cs.x : cs.y;
            cross = std::max(cross, horizontal ? cs.y : cs.x);
        }
        if (child_count() >= 2) {
            const glm::vec2 cs = Widget::measure_child(*child_at(1), constraints);
            second = horizontal ? cs.x : cs.y;
            cross = std::max(cross, horizontal ? cs.y : cs.x);
        }
        const glm::vec2 natural = horizontal
            ? glm::vec2{first + second + handle, cross}
            : glm::vec2{cross, first + second + handle};
        // 没有任何 pane 时退回构造期推荐尺寸。
        if (child_count() == 0) return {preferred_width_, preferred_height_};
        return clamp_size(natural);
    }

    void arrange(Rect slot) override
    {
        set_geometry(slot);
        const auto limits = ratio_limits();
        ratio_ = std::clamp(ratio_, limits.first, limits.second);
        const float handle = handle_size();
        if (child_count() < 2) {
            if (child_count() != 0) {
                // 单 pane 也要走一遍测量：子项（ScrollView / Table）要靠它更新内部度量。
                Widget::measure_child(*child_at(0), slot.size);
                child_at(0)->arrange(slot);
                set_content_extent(slot.size);
            }
            return;
        }

        // 先测量（让孩子项按分配到的尺寸算内部度量），再按 pane 几何安排。
        // 分配时两边都尊重子项 min_size（UI-33：ratio 由 min_size 推导）。
        if (orientation_ == Orientation::Horizontal) {
            const float usable = std::max(0.0f, slot.size.x - handle);
            const float first_w = clamp_first(resolved_first(), usable, slot.size.y);
            const glm::vec2 second_w{std::max(0.0f, usable - first_w), slot.size.y};
            Widget::measure_child(*child_at(0), {first_w, slot.size.y});
            Widget::measure_child(*child_at(1), second_w);
            child_at(0)->arrange({{slot.pos.x, slot.pos.y}, {first_w, slot.size.y}});
            child_at(1)->arrange({{slot.pos.x + first_w + handle, slot.pos.y}, second_w});
        } else {
            const float usable = std::max(0.0f, slot.size.y - handle);
            const float first_h = clamp_first(resolved_first(), usable, slot.size.x);
            const glm::vec2 second_h{slot.size.x, std::max(0.0f, usable - first_h)};
            Widget::measure_child(*child_at(0), {slot.size.x, first_h});
            Widget::measure_child(*child_at(1), second_h);
            child_at(0)->arrange({{slot.pos.x, slot.pos.y}, {slot.size.x, first_h}});
            child_at(1)->arrange({{slot.pos.x, slot.pos.y + first_h + handle}, second_h});
        }
        set_content_extent(slot.size);
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
        // UI-52：状态统一由 Interaction 推导（拖 = Pressed，hover = Hovered）。
        const ControlState bar_state = Interaction{true, is_hovered(), input.dragging}.state();
        const BoxStyle bar_style = visual_style(StyleRole::Control, bar_state);
        glm::vec4 bar_color = t.splitter;
        if (bar_state == ControlState::Pressed) {
            bar_color = with_alpha(t.accent, 0.28f);
        } else if (bar_state == ControlState::Hovered) {
            bar_color = bar_style.fill;
        }
        const float bar_radius = corner_radius(t.radius_small);
        graphics::append_rounded_rect(list, {x, y, w, h}, bar_radius, bar_color);
        // 拖动反馈：描边强调色细环。
        if (input.dragging) {
            graphics::append_rounded_ring(list, {x, y, w, h}, bar_radius, 1.0f, t.accent);
        }

        for (const auto& child : children()) {
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
        input.dragging = true;
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (!input.dragging) return false;
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
        input.dragging = false;
        return true;
    }

private:
    // 手柄厚度来自主题（UI-35 收口）：density 变化时跟着缩放。
    float handle_size() const { return std::max(1.0f, ui_theme().splitter_handle); }

    // 把第一块像素尺寸夹到 [first_min, usable - second_min]，两侧都尊重 min_size。
    float clamp_first(float first, float usable, float cross_extent) const
    {
        if (child_count() < 2) return std::clamp(first, 0.0f, usable);
        const bool horizontal = orientation_ == Orientation::Horizontal;
        const glm::vec2 a = child_at(0)->min_size();
        const glm::vec2 b = child_at(1)->min_size();
        const float first_min = horizontal ? a.x : a.y;
        const float second_min = horizontal ? b.x : b.y;
        (void)cross_extent;
        const float upper = std::max(first_min, usable - second_min);
        return std::clamp(first, std::min(first_min, upper), upper);
    }

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
};
} // namespace xel::ui
