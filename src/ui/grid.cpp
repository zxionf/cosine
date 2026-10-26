#include "grid.hpp"

#include <algorithm>
#include <vector>

namespace xel::ui
{
namespace
{
// 与 Widget::measure 的约定一致：约束 >= 该值视为无界。
constexpr float kUnbounded = 1.0e8f;

int column_count(const std::vector<float>& widths)
{
    return widths.empty() ? 1 : static_cast<int>(widths.size());
}
} // namespace

Grid::Grid(float x, float y, float w, float height)
{
    set_pos(x, y);
    set_size(w, height);
    set_padding(4.0f);
}

Grid::Grid(glm::vec2 pos, glm::vec2 size)
{
    set_pos(pos);
    set_size(size);
    set_padding(4.0f);
}

Grid::~Grid() = default;

void Grid::add(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    child->set_context(context());
    children_.push_back(std::move(child));
    mark_layout_dirty();
}

void Grid::layout()
{
    arrange(local_bounds());
}

void Grid::append_to(graphics::DrawList& list) const
{
    if (is_debug()) debug_rect(list);
    // z-index 控制同层绘制顺序（大的在上）；没有非零 z 时保持加入顺序。
    for (Widget* child : paint_order()) {
        if (child->visible()) child->append_to(list);
    }
}

void Grid::set_columns(int count)
{
    const int n = std::max(1, count);
    widths_.assign(static_cast<std::size_t>(n), -1.0f);
    mark_layout_dirty();
}

void Grid::set_column_widths(std::vector<float> widths)
{
    if (widths.empty()) widths.assign(1, -1.0f);
    widths_ = std::move(widths);
    mark_layout_dirty();
}

Align Grid::resolve_cell_align(const Widget& child) const
{
    if (auto a = child.align_self()) return *a;
    return cell_align_;
}

Grid::ColumnLayout Grid::resolve_columns(float inner_w) const
{
    const int cols = column_count(widths_);
    ColumnLayout out;
    out.offset.assign(static_cast<std::size_t>(cols), 0.0f);
    out.width.assign(static_cast<std::size_t>(cols), 0.0f);

    const float gaps = spacing_x_ * static_cast<float>(cols - 1);
    const float available = std::max(0.0f, inner_w - gaps);

    float fixed_sum = 0.0f;
    float weight_sum = 0.0f;
    for (int i = 0; i < cols; ++i) {
        const float v = widths_[static_cast<std::size_t>(i)];
        if (v > 0.0f) {
            out.width[static_cast<std::size_t>(i)] = v;
            fixed_sum += v;
        } else {
            weight_sum += -v;
        }
    }

    float remaining = available - fixed_sum;
    if (weight_sum > 0.0f) {
        if (remaining <= 0.0f) {
            // 固定列已吃满（或超出）可用宽：等比压缩固定列，弹性列归零。
            const float scale = available / std::max(fixed_sum, 1.0e-6f);
            for (int i = 0; i < cols; ++i) {
                if (widths_[static_cast<std::size_t>(i)] > 0.0f) {
                    out.width[static_cast<std::size_t>(i)] *= scale;
                }
            }
        } else {
            for (int i = 0; i < cols; ++i) {
                const float v = widths_[static_cast<std::size_t>(i)];
                if (v <= 0.0f) {
                    out.width[static_cast<std::size_t>(i)] = remaining * (-v) / weight_sum;
                }
            }
        }
    } else if (remaining < 0.0f) {
        // 全是固定列且超宽：等比压缩，避免负宽。
        const float scale = available / std::max(fixed_sum, 1.0e-6f);
        for (int i = 0; i < cols; ++i) {
            out.width[static_cast<std::size_t>(i)] *= scale;
        }
    }

    float x = 0.0f;
    for (int i = 0; i < cols; ++i) {
        out.offset[static_cast<std::size_t>(i)] = x;
        x += out.width[static_cast<std::size_t>(i)] + spacing_x_;
    }
    return out;
}

glm::vec2 Grid::measure(glm::vec2 constraints)
{
    if (measure_valid_ && constraints == cached_constraints_) {
        return cached_measure_;
    }

    const int cols = column_count(widths_);

    // 绝对定位子项不占格：行数只按流式子项计算。
    std::size_t flow_count = 0;
    for (const auto& child : children_) {
        if (!child->absolute()) ++flow_count;
    }
    const int rows = flow_count == 0
        ? 0
        : static_cast<int>((flow_count + static_cast<std::size_t>(cols) - 1) / static_cast<std::size_t>(cols));

    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = std::max(0.0f, constraints.x - pad_l - pad_r);

    ColumnLayout layout;
    const bool unbounded_w = constraints.x >= kUnbounded;
    if (unbounded_w) {
        // 无界约束下弹性列取该列子项的自然宽最大值，使「内容自适应」可用。
        std::vector<float> natural(static_cast<std::size_t>(cols), 0.0f);
        std::size_t flow = 0;
        for (std::size_t i = 0; i < children_.size(); ++i) {
            const auto& child = children_[i];
            if (child->absolute()) continue;
            const int col = static_cast<int>(flow % static_cast<std::size_t>(cols));
            ++flow;
            const glm::vec2 cs = Widget::measure_child(*child, {kUnbounded, kUnbounded});
            const float need = child->margin_left() + cs.x + child->margin_right();
            natural[static_cast<std::size_t>(col)] =
                std::max(natural[static_cast<std::size_t>(col)], need);
        }
        layout.width.assign(static_cast<std::size_t>(cols), 0.0f);
        layout.offset.assign(static_cast<std::size_t>(cols), 0.0f);
        float x = 0.0f;
        for (int i = 0; i < cols; ++i) {
            const std::size_t ci = static_cast<std::size_t>(i);
            layout.width[ci] = widths_[ci] > 0.0f ? widths_[ci] : natural[ci];
            layout.offset[ci] = x;
            x += layout.width[ci] + spacing_x_;
        }
    } else {
        layout = resolve_columns(inner_w);
    }

    std::vector<float> row_h(static_cast<std::size_t>(rows), 0.0f);
    const float inner_h = constraints.y >= kUnbounded
        ? kUnbounded
        : std::max(0.0f, constraints.y - pad_t - pad_b);

    std::size_t flow = 0;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto& child = children_[i];
        if (child->absolute()) continue;
        const int row = static_cast<int>(flow / static_cast<std::size_t>(cols));
        const int col = static_cast<int>(flow % static_cast<std::size_t>(cols));
        ++flow;
        const std::size_t ci = static_cast<std::size_t>(col);

        const float cell_w = std::max(0.0f, layout.width[ci] - child->margin_left() - child->margin_right());
        const glm::vec2 cs = Widget::measure_child(*child, {cell_w, inner_h});
        const float need = child->margin_top() + cs.y + child->margin_bottom();
        row_h[static_cast<std::size_t>(row)] =
            std::max(row_h[static_cast<std::size_t>(row)], need);
    }

    float total_w = pad_l + pad_r + spacing_x_ * static_cast<float>(cols - 1);
    for (float w : layout.width) total_w += w;

    float total_h = pad_t + pad_b + spacing_y_ * static_cast<float>(rows > 0 ? rows - 1 : 0);
    for (float h : row_h) total_h += h;

    cached_measure_ = clamp_size({total_w, total_h});
    cached_constraints_ = constraints;
    measure_valid_ = true;
    return cached_measure_;
}

void Grid::arrange(Rect slot)
{
    global_ = slot;
    if (children_.empty()) return;

    const int cols = column_count(widths_);
    const float pad_l = padding_left();
    const float pad_r = padding_right();
    const float pad_t = padding_top();
    const float pad_b = padding_bottom();
    const float inner_w = std::max(0.0f, slot.size.x - pad_l - pad_r);
    const float inner_h = std::max(0.0f, slot.size.y - pad_t - pad_b);

    const ColumnLayout layout = resolve_columns(inner_w);

    // 流式子项索引：绝对定位子项不占格。
    std::vector<std::size_t> flow;
    flow.reserve(children_.size());
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (!children_[i]->absolute()) flow.push_back(i);
    }
    const int rows = flow.empty()
        ? 0
        : static_cast<int>((flow.size() + static_cast<std::size_t>(cols) - 1) / static_cast<std::size_t>(cols));

    // 先按列宽测量一次，得到每格内容尺寸与行高（行高 = 行内最大内容高）。
    std::vector<glm::vec2> cell_size(flow.size(), glm::vec2{0.0f, 0.0f});
    std::vector<float> row_h(static_cast<std::size_t>(rows), 0.0f);

    for (std::size_t k = 0; k < flow.size(); ++k) {
        const auto& child = children_[flow[k]];
        const int row = static_cast<int>(k / static_cast<std::size_t>(cols));
        const int col = static_cast<int>(k % static_cast<std::size_t>(cols));
        const std::size_t ci = static_cast<std::size_t>(col);

        const float cell_w =
            std::max(0.0f, layout.width[ci] - child->margin_left() - child->margin_right());
        glm::vec2 cs = Widget::measure_child(*child, {cell_w, kUnbounded});
        cs.x = std::min(cs.x, cell_w);
        cell_size[k] = cs;
        row_h[static_cast<std::size_t>(row)] = std::max(
            row_h[static_cast<std::size_t>(row)],
            child->margin_top() + cs.y + child->margin_bottom());
    }

    float y = slot.pos.y + pad_t;
    for (int r = 0; r < rows; ++r) {
        const float cell_h = row_h[static_cast<std::size_t>(r)];
        for (int c = 0; c < cols; ++c) {
            const std::size_t k = static_cast<std::size_t>(r) * static_cast<std::size_t>(cols)
                + static_cast<std::size_t>(c);
            if (k >= flow.size()) break;

            auto& child = children_[flow[k]];
            const std::size_t ci = static_cast<std::size_t>(c);
            const Align a = resolve_cell_align(*child);

            const float avail_w = std::max(
                0.0f, layout.width[ci] - child->margin_left() - child->margin_right());
            const float avail_h =
                std::max(0.0f, cell_h - child->margin_top() - child->margin_bottom());

            glm::vec2 size = cell_size[k];
            float cx = 0.0f;
            float cy = 0.0f;

            if (a.stretch_x && child->size_percent().x <= 0.0f) {
                size.x = avail_w;
                cx = layout.offset[ci] + child->margin_left();
            } else {
                size.x = std::min(size.x, avail_w);
                cx = layout.offset[ci] + child->margin_left() + (avail_w - size.x) * a.x;
            }

            if (a.stretch_y && child->size_percent().y <= 0.0f) {
                size.y = avail_h;
                cy = child->margin_top();
            } else {
                size.y = std::min(size.y, avail_h);
                cy = child->margin_top() + (avail_h - size.y) * a.y;
            }

            child->arrange({{slot.pos.x + pad_l + cx, y + cy}, size});
        }
        y += cell_h + spacing_y_;
    }

    // 绝对定位子项：按自身坐标相对内容区放置，尺寸走同一套测量规则。
    for (const auto& child : children_) {
        if (!child->absolute()) continue;
        const glm::vec2 cs = Widget::measure_child(*child, {inner_w, inner_h});
        child->arrange({{slot.pos.x + pad_l + child->x(), slot.pos.y + pad_t + child->y()}, cs});
    }
}
} // namespace xel::ui
