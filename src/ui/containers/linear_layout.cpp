#include "ui/containers/linear_layout.hpp"

#include "ui/layout_budget.hpp"
#include "ui/layout_metrics.hpp"
#include "ui/layout_utils.hpp"
#include "ui/uicontext.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
LinearLayout::LinearLayout(Axis axis) : axis_(axis)
{
    // 默认内边距 4：结构性默认值，不锁主题（见 Widget::set_default_padding）。
    set_default_padding(4.0f);
}

void LinearLayout::set_axis_defaults(Align cross_align)
{
    cross_align_ = cross_align;
}

void LinearLayout::set_context(UIContext* context)
{
    Widget::set_context(context);
    if (context == nullptr) return;
    apply_theme_padding(context->theme.padding);
    apply_theme_spacing(context->theme.spacing);
}

void LinearLayout::apply_theme_spacing(float spacing)
{
    if (spacing_override_ || spacing_ == spacing) return;
    spacing_ = spacing;
    mark_layout_dirty();
}

void LinearLayout::add(std::unique_ptr<Widget> child)
{
    adopt(std::move(child));
}

void LinearLayout::layout()
{
    // 重排用最近一次 arrange 的矩形，不要用 local_（历史坑：local_ 陈旧会把布局打回构造尺寸）。
    arrange(global_);
}

void LinearLayout::append_to(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    // z-index 控制同层绘制顺序（大的在上）；没有非零 z 时保持加入顺序。
    for (Widget* child : paint_order()) {
        if (child->visible()) child->append_to(list);
    }
}

Align LinearLayout::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

glm::vec2 LinearLayout::measure(glm::vec2 constraints)
{
    if (measure_cache_hit(constraints)) {
        return measure_cached();
    }

    const bool horizontal = axis_ == Axis::Horizontal;
    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = is_bounded(constraints.x)
        ? std::max(0.0f, constraints.x - pad_l - pad_r)
        : kUnbounded;
    const float inner_h = is_bounded(constraints.y)
        ? std::max(0.0f, constraints.y - pad_t - pad_b)
        : kUnbounded;

    std::vector<MainAxisItem> items;
    float cross_max = 0.0f;

    for (const auto& child : children_) {
        // 隐藏子项不参与流式尺寸；绝对定位子项也不参与。
        if (child->display() != Display::Flow) {
            child->mark_measured();
            continue;
        }

        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, inner_h});
        MainAxisItem item;
        item.pct = horizontal ? child->size_percent().x : child->size_percent().y;
        item.fill = horizontal ? child->fill_x() : child->fill_y();
        item.fixed = child->fixed_size();
        item.flex = child->flex_grow();
        const float natural = horizontal ? cs.x : cs.y;
        // flex 子项主轴只按 min_size 计价：内容比容器大时不把容器自然尺寸撑爆。
        item.natural = child->flex_grow() > 0.0f
            ? std::max(horizontal ? child->min_size().x : child->min_size().y, 0.0f)
            : natural;
        item.margin_before = horizontal ? child->margin_left() : child->margin_top();
        item.margin_after = horizontal ? child->margin_right() : child->margin_bottom();
        item.min_size = horizontal ? child->min_size().x : child->min_size().y;
        item.max_size = horizontal ? child->max_size().x : child->max_size().y;
        items.push_back(item);

        const float cross = horizontal ? (child->margin_top() + cs.y + child->margin_bottom())
                                       : (child->margin_left() + cs.x + child->margin_right());
        cross_max = std::max(cross_max, cross);
    }

    const float available = horizontal ? inner_w : inner_h;
    const std::vector<float> main = resolve_main_axis(available, spacing_, items, false);

    float total = spacing_ * static_cast<float>(items.empty() ? 0 : items.size() - 1);
    for (std::size_t i = 0; i < items.size(); ++i) {
        total += items[i].margin_before + main[i] + items[i].margin_after;
    }

    // content_extent 只在 arrange 写（真实占用）：measure 不应污染诊断数据。
    const glm::vec2 extent = horizontal ? glm::vec2{total, cross_max} : glm::vec2{cross_max, total};
    const glm::vec2 result = clamp_size(extent + glm::vec2{pad_l + pad_r, pad_t + pad_b});
    store_measure(constraints, result);
    return result;
}

void LinearLayout::arrange(Rect slot)
{
    set_geometry(slot);
    if (children_.empty()) return;

    const bool horizontal = axis_ == Axis::Horizontal;
    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();

    const float inner_x = slot.pos.x + pad_l;
    const float inner_y = slot.pos.y + pad_t;
    const float inner_w = std::max(0.0f, slot.size.x - pad_l - pad_r);
    const float inner_h = std::max(0.0f, slot.size.y - pad_t - pad_b);
    const float available = horizontal ? inner_w : inner_h;

    struct Item
    {
        Widget* child;
        glm::vec2 cross_size;
        float margin_before, margin_after, cross_before, cross_after;
    };

    std::vector<Item> measured;
    std::vector<MainAxisItem> specs;
    measured.reserve(children_.size());

    for (const auto& child : children_) {
        // 流式子项：隐藏与绝定位的子项不占位、不参与 spacing / flex 分配。
        if (child->display() != Display::Flow) continue;

        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, inner_h});
        MainAxisItem item;
        item.pct = horizontal ? child->size_percent().x : child->size_percent().y;
        item.fill = horizontal ? child->fill_x() : child->fill_y();
        item.fixed = child->fixed_size();
        item.flex = child->flex_grow();
        const float natural = horizontal ? cs.x : cs.y;
        item.natural = child->flex_grow() > 0.0f
            ? std::max(horizontal ? child->min_size().x : child->min_size().y, 0.0f)
            : natural;
        item.margin_before = horizontal ? child->margin_left() : child->margin_top();
        item.margin_after = horizontal ? child->margin_right() : child->margin_bottom();
        item.min_size = horizontal ? child->min_size().x : child->min_size().y;
        item.max_size = horizontal ? child->max_size().x : child->max_size().y;
        specs.push_back(item);

        measured.push_back({child.get(), cs,
                            item.margin_before, item.margin_after,
                            horizontal ? child->margin_top() : child->margin_left(),
                            horizontal ? child->margin_bottom() : child->margin_right()});
    }

    const std::vector<float> main = resolve_main_axis(available, spacing_, specs);

    // 自然主轴总长（含 margin 与间距），用于主轴对齐。
    float natural = spacing_ * static_cast<float>(measured.empty() ? 0 : measured.size() - 1);
    for (std::size_t i = 0; i < measured.size(); ++i) {
        natural += measured[i].margin_before + main[i] + measured[i].margin_after;
    }
    const float extra = available - natural;

    float cursor = horizontal ? inner_x : inner_y;
    float gap = spacing_;
    switch (main_align_) {
        case MainAlign::Start:
            break;
        case MainAlign::Center:
            cursor += std::max(0.0f, extra) * 0.5f;
            break;
        case MainAlign::End:
            cursor += std::max(0.0f, extra);
            break;
        case MainAlign::SpaceBetween:
            if (measured.size() > 1 && extra > 0.0f) {
                gap += extra / static_cast<float>(measured.size() - 1);
            }
            break;
    }

    const float cross_origin = horizontal ? inner_y : inner_x;
    const float cross_extent = horizontal ? inner_h : inner_w;

    // 基线对齐（UI-36）：水平流下先算出本行最大基线，再逐项对齐。
    float row_baseline = 0.0f;
    if (horizontal) {
        for (auto& m : measured) {
            const Align a = resolve_align(*m.child);
            if (!a.baseline) continue;
            row_baseline = std::max(row_baseline,
                                    m.cross_before + m.child->baseline_offset_for(m.cross_size.y));
        }
    }

    for (std::size_t i = 0; i < measured.size(); ++i) {
        Widget& child = *measured[i].child;
        const Align a = resolve_align(child);
        const float cross_available = std::max(
            0.0f, cross_extent - measured[i].cross_before - measured[i].cross_after);
        const bool pct_cross = horizontal
            ? (child.size_percent().y > 0.0f || child.fill_y())
            : (child.size_percent().x > 0.0f || child.fill_x());
        const bool stretch = (horizontal ? a.stretch_y : a.stretch_x)
            && !pct_cross && !child.fixed_size();

        glm::vec2 size = measured[i].cross_size;
        if (horizontal) {
            size.x = main[i];
            if (stretch) size.y = cross_available;
        } else {
            size.y = main[i];
            if (stretch) size.x = cross_available;
        }

        // stretch 后重 clamp；aspect 与 clamp 一起求解（UI-32）。
        const float ratio = child.aspect_ratio();
        if (ratio > 0.0f) {
            const bool main_is_pct = horizontal
                ? (child.size_percent().x > 0.0f || child.fill_x())
                : (child.size_percent().y > 0.0f || child.fill_y());
            if (main_is_pct && !pct_cross) {
                if (horizontal) size.y = size.x / ratio;
                else size.x = size.y * ratio;
            } else if (horizontal) {
                size.y = size.x / ratio;
            } else {
                size.x = size.y * ratio;
            }
            size = apply_aspect_clamp(size, ratio, child.min_size(), child.max_size());
        } else {
            size = child.clamp_size(size);
        }

        const float cross_pos = stretch
            ? cross_origin + measured[i].cross_before
            : (horizontal && a.baseline)
                ? cross_origin + row_baseline - child.baseline_offset_for(size.y)
                : cross_origin + measured[i].cross_before
                    + (cross_available - (horizontal ? size.y : size.x))
                        * (horizontal ? a.y : a.x);
        const float main_pos = cursor + measured[i].margin_before;

        child.arrange(horizontal ? Rect{{main_pos, cross_pos}, size}
                                 : Rect{{cross_pos, main_pos}, size});
        cursor += measured[i].margin_before + main[i] + measured[i].margin_after + gap;
    }

    // 内容实际占用尺寸 = 子项包围盒相对自身原点的范围（UI-36）：
    // 用于 content_overflows() 诊断，取真实几何而不是自然尺寸。
    glm::vec2 occupied{0.0f, 0.0f};
    for (const auto& child : children_) {
        if (child->display() == Display::Hidden) continue;
        const auto b = child->global_bounds();
        occupied.x = std::max(occupied.x, b.pos.x + b.size.x - slot.pos.x);
        occupied.y = std::max(occupied.y, b.pos.y + b.size.y - slot.pos.y);
    }
    set_content_extent(occupied);

    // 绝对定位子项：按自身坐标相对内容区放置，尺寸走同一套测量规则。
    layout::place_positioned_children(*this, layout::content_rect(*this, slot), {inner_w, inner_h});
}
} // namespace xel::ui
