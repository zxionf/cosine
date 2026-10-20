#include "scroll_view.hpp"

namespace xel::ui
{
void ScrollView::set_content(std::unique_ptr<Widget> c)
{
    if (content_) {
        content_->~Widget();          // 或 remove_child
    }
    content_ = std::move(c);
    // content_->set_parent(this);       // 需要 parent 指针，用于冒泡
    // mark_layout_dirty();
}

glm::vec2 ScrollView::measure(glm::vec2 constraints)
{
    if (!content_) return constraints;

    glm::vec2 inf = {constraints.x, 1e9f};
    content_size_ = content_->measure(inf);

    return {
        std::min(content_size_.x, constraints.x),
        std::min(content_size_.y, constraints.y)
    };
}

void ScrollView::arrange(Rect slot)
{
    global_ = slot;
    if (!content_) return;

    // Rect inner = content_bounds();    // 去掉 padding
    Rect inner = slot;

    // ★ 把 content 放到 (inner.x, inner.y - offset) 处
    // content 超出 inner 的部分会被裁掉（渲染时 clip）
    content_->arrange({
        {inner.pos.x, inner.pos.y - offset_},
        {content_size_.x, content_size_.y}
    });
}

bool ScrollView::on_scroll(glm::vec2 pos, float delta)
{
    // 只处理落在自己 viewport 内的滚动
    if (!global_bounds().contains(pos)) return false;

    float max_offset = std::max(0.f, content_size_.y - global_.size.y);

    // ★ 方向反转：向下滚 → 内容向上移 → offset 增大
    offset_ += delta * 40.f;     // 40 是灵敏度，可调
    offset_ = std::clamp(offset_, 0.f, max_offset);

    // mark_layout_dirty();
    return true;   // ★ 消费，不再冒泡给外层 ScrollView
}

void ScrollView::append_to(graphics::DrawBatch& batch) const
{
    content_->append_to(batch);
}
} // namespace xel::ui