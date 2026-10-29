#include "ui/containers/wrap.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
namespace
{
// 与 Widget::measure 的约定一致：约束 >= 该值视为无界。
constexpr float kUnbounded = 1.0e8f;
} // namespace

Wrap::Wrap(float x, float y, float w, float height)
{
    set_pos(x, y);
    set_size(w, height);
    set_padding(4.0f);
}

Wrap::Wrap(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
    set_padding(4.0f);
}

Wrap::~Wrap() = default;

void Wrap::add(std::unique_ptr<Widget> child)
{
    adopt(std::move(child));
}

void Wrap::layout()
{
    arrange(local_bounds());
}

void Wrap::append_to(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    // z-index 控制同层绘制顺序（大的在上）；没有非零 z 时保持加入顺序。
    for (Widget* child : paint_order()) {
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
    if (children_.empty()) return rows;

    Row row;
    float used = 0.0f; // 当前行已占宽（含 margin 与间距）

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto& child = children_[i];
        // 绝对定位子项不参与折行。
        if (child->absolute()) continue;
        const float ml = child->margin_left();
        const float mr = child->margin_right();
        const float mt = child->margin_top();
        const float mb = child->margin_bottom();

        const glm::vec2 cs = Widget::measure_child(
            *child, {std::max(0.0f, inner_w - ml - mr), kUnbounded});
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
    if (measure_valid_ && constraints == cached_constraints_) {
        return cached_measure_;
    }

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = std::max(0.0f, constraints.x - pad_l - pad_r);
    const bool unbounded = constraints.x >= kUnbounded;

    const std::vector<Row> rows = layout_rows(inner_w, unbounded);

    float max_w = 0.0f;
    float total_h = pad_t + pad_b;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) total_h += spacing_y_;
        max_w = std::max(max_w, rows[i].width);
        total_h += rows[i].height;
    }

    // 折行依赖可用宽：有限约束下直接返回可用宽（内容更宽时返回内容宽），
    // 保证父容器按 measure 结果 arrange 时宽度不缩、折行结果与 measure 一致。
    const float content_w = max_w + pad_l + pad_r;
    const float total_w = unbounded ? content_w : std::max(constraints.x, content_w);

    cached_measure_ = clamp_size({total_w, total_h});
    cached_constraints_ = constraints;
    measure_valid_ = true;
    return cached_measure_;
}

void Wrap::arrange(Rect slot)
{
    global_ = slot;
    if (children_.empty()) return;

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float inner_x = slot.pos.x + pad_l;
    const float inner_w = std::max(0.0f, slot.size.x - pad_l - pad_r);
    const bool unbounded = inner_w >= kUnbounded;

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
            auto& child = children_[idx];
            const Align a = resolve_align(*child);

            const float ml = child->margin_left();
            const float mt = child->margin_top();
            const float mr = child->margin_right();
            const float mb = child->margin_bottom();

            glm::vec2 size = Widget::measure_child(
                *child, {std::max(0.0f, inner_w - ml - mr), kUnbounded});

            const float avail_h = std::max(0.0f, row.height - mt - mb);
            float cy = 0.0f;
            if (a.stretch_y && child->size_percent().y <= 0.0f) {
                size.y = avail_h;
                cy = y + mt;
            } else {
                size.y = std::min(size.y, avail_h);
                cy = y + mt + (avail_h - size.y) * a.y;
            }

            // 水平位置由流式排布决定：Wrap 不按 a.x 拉伸/偏移子项。
            child->arrange({{x + ml, cy}, size});
            x += ml + size.x + mr + gap;
        }

        y += row.height + spacing_y_;
    }

    // 绝对定位子项：按自身坐标相对内容区放置，尺寸走同一套测量规则。
    for (const auto& child : children_) {
        if (!child->absolute()) continue;
        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, slot.size.y});
        child->arrange({{inner_x + child->absolute_pos().x,
                         slot.pos.y + pad_t + child->absolute_pos().y}, cs});
    }
}
} // namespace xel::ui
