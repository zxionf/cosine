#include "ui/containers/wrap.hpp"

#include "ui/geom/layout_metrics.hpp"
#include "ui/layout/layout_utils.hpp"
#include "ui/system/uicontext.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
namespace
{} // namespace

Wrap::Wrap(float x, float y, float w, float height)
{
    set_pos(x, y);
    set_size(w, height);
    // 结构性默认值，不锁主题。
    set_default_padding(4.0f);
    // 流式容器横向铺满父级：换行以父宽为准。
    set_fill(true, false);
}

Wrap::Wrap(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
    set_default_padding(4.0f);
    set_fill(true, false);
}

Wrap::~Wrap() = default;

void Wrap::set_context(UIContext* context)
{
    Widget::set_context(context);
    if (context == nullptr) return;
    apply_theme_padding(context->theme().padding);
    apply_theme_spacing(context->theme().spacing);
}

void Wrap::add(std::unique_ptr<Node> child)
{
    adopt(std::move(child));
}

void Wrap::layout()
{
    arrange(bounds_);
}

void Wrap::append_to(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    // z-index 控制同层绘制顺序（大的在上）；没有非零 z 时保持加入顺序。
    for (Node* child : paint_order()) {
        if (child->visible()) child->append_to(list);
    }
}

Align Wrap::resolve_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cross_align_;
}

std::vector<Wrap::Row> Wrap::layout_rows(float inner_w, bool unbounded)
{
    std::vector<Row> rows;
    if (child_count() == 0) return rows;

    Row row;
    float used = 0.0f; // 当前行已占宽（含 margin 与间距）

    for (std::size_t i = 0; i < child_count(); ++i) {
        auto* child = child_at(i);
        // 隐藏子项与绝对定位子项不参与折行。
        if (child->display() != Display::Flow) {
            child->mark_measured();
            continue;
        }
        const float ml = child->margin_left();
        const float mr = child->margin_right();
        const float mt = child->margin_top();
        const float mb = child->margin_bottom();

        // margin 由 measure_child 内部扣，这里传入容器内容区。
        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, kUnbounded});
        const float need = ml + cs.x + mr;
        const float item_h = mt + cs.y + mb;

        const bool row_empty = row.items.empty();
        const bool overflows = !unbounded && !row_empty
            && used + spacing_x_ + need > inner_w;

        if (overflows) {
            rows.push_back(std::move(row));
            row = Row{};
            row.items.push_back(i);
            row.width = need;
            row.height = item_h;
            used = need;
        } else {
            if (!row_empty) {
                row.width += spacing_x_;
                used += spacing_x_;
            }
            row.items.push_back(i);
            row.width += need;
            row.height = std::max(row.height, item_h);
            used += need;
        }
    }

    if (!row.items.empty()) rows.push_back(std::move(row));
    return rows;
}

glm::vec2 Wrap::measure(glm::vec2 constraints)
{
    if (measure_cache_hit(constraints)) {
        return measure_cached();
    }

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = std::max(0.0f, constraints.x - pad_l - pad_r);
    const bool unbounded = !is_bounded(constraints.x);

    const std::vector<Row> rows = layout_rows(inner_w, unbounded);

    float max_w = 0.0f;
    float total_h = pad_t + pad_b;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) total_h += spacing_y_;
        max_w = std::max(max_w, rows[i].width);
        total_h += rows[i].height;
    }

    // Wrap 是块级容器：有界时占满可用宽（既有约定，见 test_ui_wrap），
    // 但绝不超过 constraints（旧实现取 max，子项超宽时会报 300 > 200）。
    const float content_w = max_w + pad_l + pad_r;
    const float total_w = unbounded ? content_w : constraints.x;

    // content_extent 只在 arrange 写（真实占用）。
    const glm::vec2 result = clamp_size({total_w, total_h});
    store_measure(constraints, result);
    return result;
}

void Wrap::arrange(Rect slot)
{
    set_geometry(slot);
    if (child_count() == 0) return;

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float inner_x = slot.pos.x + pad_l;
    const float inner_w = std::max(0.0f, slot.size.x - pad_l - pad_r);
    const bool unbounded = !is_bounded(inner_w);

    const std::vector<Row> rows = layout_rows(inner_w, unbounded);

    float y = slot.pos.y + pad_t;
    for (const auto& row : rows) {
        const float extra = inner_w - row.width;
        const float count = static_cast<float>(row.items.size());
        float gap = spacing_x_;
        float x = inner_x;

        switch (row_align_) {
            case MainAlign::Start:
                break;
            case MainAlign::Center:
                x += std::max(0.0f, extra) * 0.5f;
                break;
            case MainAlign::End:
                x += std::max(0.0f, extra);
                break;
            case MainAlign::SpaceBetween:
                if (row.items.size() > 1 && extra > 0.0f) {
                    gap += extra / (count - 1.0f);
                }
                break;
        }

        for (std::size_t idx : row.items) {
            auto* child = child_at(idx);
            const Align a = resolve_align(*child);

            const float ml = child->margin_left();
            const float mt = child->margin_top();
            const float mr = child->margin_right();
            const float mb = child->margin_bottom();

            // margin 由 measure_child 内部扣。
            glm::vec2 size = Widget::measure_child(*child, {inner_w, kUnbounded});

            const float avail_h = std::max(0.0f, row.height - mt - mb);
            float cy = 0.0f;
            const bool pct_y = child->size_percent().y > 0.0f || child->fill_y();
            const bool stretch_y = a.stretch_y && !pct_y && !child->fixed_size();
            if (stretch_y) {
                size.y = avail_h;
                cy = y + mt;
            } else {
                size.y = std::min(size.y, avail_h);
                cy = y + mt + (avail_h - size.y) * a.y;
            }

            // stretch 后重 clamp，aspect 与 clamp 一起求解。
            const float ratio = child->aspect_ratio();
            if (ratio > 0.0f) {
                size = apply_aspect_clamp(size, ratio, child->min_size(), child->max_size());
            } else {
                size = child->clamp_size(size);
            }

            // 水平位置由流式排布决定：Wrap 不按 a.x 拉伸/偏移子项。
            child->arrange({{x + ml, cy}, size});
            x += ml + size.x + mr + gap;
        }

        y += row.height + spacing_y_;
    }

    set_content_extent({inner_w, y - slot.pos.y - pad_t});

    // 绝对定位子项：按自身坐标相对内容区放置，尺寸走同一套测量规则。
    layout::place_positioned_children(*this, layout::content_rect(*this, slot),
                                      {inner_w, slot.size.y});
}
} // namespace xel::ui
