#include "scroll_view.hpp"

#include "../graphics/rectangle.hpp"

#include <algorithm>

namespace xel::ui
{
void ScrollView::set_content(std::unique_ptr<Widget> c)
{
    children_.clear();
    content_ = nullptr;
    if (c) {
        c->set_parent(this);
        c->set_context(context());
        content_ = c.get();
        children_.push_back(std::move(c));
    }
    mark_layout_dirty();
}

glm::vec2 ScrollView::content_constraints(glm::vec2 constraints) const
{
    constexpr float kInf = 1e9f;
    switch (axis_) {
        case Axis::Vertical:   return {constraints.x, kInf};
        case Axis::Horizontal: return {kInf, constraints.y};
        case Axis::Both:       return {kInf, kInf};
    }
    return constraints;
}

void ScrollView::update_metrics(glm::vec2 outer)
{
    const auto& t = ui_theme();
    const float bar = t.scrollbar_width + 2.0f;

    viewport_ = outer;
    content_size_ = content_ ? content_->measure(content_constraints(viewport_)) : glm::vec2{};
    show_v_ = false;
    show_h_ = false;
    if (!content_) return;

    bool need_v = (axis_ == Axis::Vertical || axis_ == Axis::Both)
        && content_size_.y > viewport_.y + 0.5f;
    bool need_h = (axis_ == Axis::Horizontal || axis_ == Axis::Both)
        && content_size_.x > viewport_.x + 0.5f;

    if (need_v || need_h) {
        glm::vec2 reduced = outer;
        if (need_v) reduced.x = std::max(1.0f, reduced.x - bar);
        if (need_h) reduced.y = std::max(1.0f, reduced.y - bar);

        // 宽度变化可能影响换行内容，重新测量后再判定一次。
        content_size_ = content_->measure(content_constraints(reduced));
        viewport_ = reduced;
        need_v = (axis_ == Axis::Vertical || axis_ == Axis::Both)
            && content_size_.y > viewport_.y + 0.5f;
        need_h = (axis_ == Axis::Horizontal || axis_ == Axis::Both)
            && content_size_.x > viewport_.x + 0.5f;
    }

    show_v_ = need_v;
    show_h_ = need_h;
}

float ScrollView::max_scroll(bool vertical) const
{
    if (vertical) {
        if (!show_v_) return 0.0f;
        return std::max(0.0f, content_size_.y - viewport_.y);
    }
    if (!show_h_) return 0.0f;
    return std::max(0.0f, content_size_.x - viewport_.x);
}

glm::vec2 ScrollView::clamp_offset(glm::vec2 offset) const
{
    return {
        std::clamp(offset.x, 0.0f, max_scroll(false)),
        std::clamp(offset.y, 0.0f, max_scroll(true))
    };
}

void ScrollView::set_scroll_offset(glm::vec2 offset)
{
    offset_ = clamp_offset(offset);
    place_content();
    mark_layout_dirty();
}

bool ScrollView::scrollbar_visible(bool vertical) const
{
    return vertical ? show_v_ : show_h_;
}

Rect ScrollView::scrollbar_track_rect(bool vertical) const
{
    const auto& t = ui_theme();
    const float width = t.scrollbar_width;
    const float margin = 2.0f;

    if (vertical) {
        const float reserved_h = show_h_ ? width + margin : 0.0f;
        return {
            {global_.pos.x + global_.size.x - width - margin, global_.pos.y + margin},
            {width, std::max(0.0f, global_.size.y - margin * 2.0f - reserved_h)}
        };
    }
    const float reserved_w = show_v_ ? width + margin : 0.0f;
    return {
        {global_.pos.x + margin, global_.pos.y + global_.size.y - width - margin},
        {std::max(0.0f, global_.size.x - margin * 2.0f - reserved_w), width}
    };
}

float ScrollView::thumb_length(bool vertical, const Rect& track) const
{
    const auto& t = ui_theme();
    const float viewport = vertical ? viewport_.y : viewport_.x;
    const float content = vertical ? content_size_.y : content_size_.x;
    if (content <= 0.0f) return vertical ? track.size.y : track.size.x;

    const float track_length = vertical ? track.size.y : track.size.x;
    const float ratio = std::clamp(viewport / content, 0.0f, 1.0f);
    return std::clamp(track_length * ratio, t.scrollbar_min_thumb, track_length);
}

Rect ScrollView::scrollbar_thumb_rect(bool vertical) const
{
    const Rect track = scrollbar_track_rect(vertical);
    const float length = thumb_length(vertical, track);
    const float max = max_scroll(vertical);
    const float offset = vertical ? offset_.y : offset_.x;
    const float t = max > 0.0f ? std::clamp(offset / max, 0.0f, 1.0f) : 0.0f;

    if (vertical) {
        return {track.pos + glm::vec2{0.0f, (track.size.y - length) * t}, {track.size.x, length}};
    }
    return {track.pos + glm::vec2{(track.size.x - length) * t, 0.0f}, {length, track.size.y}};
}

glm::vec2 ScrollView::measure(glm::vec2 constraints)
{
    update_metrics(constraints);
    return viewport_;
}

void ScrollView::place_content()
{
    if (!content_) return;
    content_->arrange({
        {global_.pos.x - offset_.x, global_.pos.y - offset_.y},
        content_size_
    });
}

void ScrollView::arrange(Rect slot)
{
    global_ = slot;
    update_metrics(slot.size);
    offset_ = clamp_offset(offset_);
    place_content();
}

bool ScrollView::on_scroll(glm::vec2 pos, glm::vec2 delta)
{
    if (!global_bounds().contains(pos)) return false;

    if (axis_ == Axis::Vertical || axis_ == Axis::Both) {
        offset_.y -= delta.y * 40.0f;
    }
    if (axis_ == Axis::Horizontal || axis_ == Axis::Both) {
        offset_.x -= delta.x * 40.0f;
    }
    offset_ = clamp_offset(offset_);
    place_content();
    mark_layout_dirty();
    return true;
}

bool ScrollView::on_mouse_down(glm::vec2 pos, int button)
{
    if (button != 0) return false;

    // 先命中滑块，再命中轨道（翻页）。
    for (bool vertical : {true, false}) {
        if (!scrollbar_visible(vertical)) continue;
        const Rect thumb = scrollbar_thumb_rect(vertical);
        if (thumb.contains(pos)) {
            dragging_ = vertical ? Drag::Vertical : Drag::Horizontal;
            drag_start_offset_ = vertical ? offset_.y : offset_.x;
            drag_start_pos_ = vertical ? pos.y : pos.x;
            return true;
        }
        const Rect track = scrollbar_track_rect(vertical);
        if (track.contains(pos)) {
            const float viewport = vertical ? viewport_.y : viewport_.x;
            const float direction = vertical
                ? (pos.y < thumb.pos.y ? -1.0f : 1.0f)
                : (pos.x < thumb.pos.x ? -1.0f : 1.0f);
            const float delta = viewport * 0.9f * direction;
            if (vertical) offset_.y += delta;
            else          offset_.x += delta;
            offset_ = clamp_offset(offset_);
            place_content();
            mark_layout_dirty();
            return true;
        }
    }
    return false;
}

bool ScrollView::on_mouse_move(glm::vec2 pos)
{
    if (dragging_ == Drag::None) return false;

    const bool vertical = dragging_ == Drag::Vertical;
    const Rect track = scrollbar_track_rect(vertical);
    const float length = thumb_length(vertical, track);
    const float track_length = vertical ? track.size.y : track.size.x;
    const float travel = std::max(1.0f, track_length - length);
    const float max = max_scroll(vertical);
    if (max <= 0.0f) return false;

    const float current = vertical ? pos.y : pos.x;
    const float delta = current - drag_start_pos_;
    const float value = std::clamp(drag_start_offset_ + delta * (max / travel), 0.0f, max);

    if (vertical) offset_.y = value;
    else          offset_.x = value;
    place_content();
    mark_layout_dirty();
    return true;
}

bool ScrollView::on_mouse_up(glm::vec2, int)
{
    if (dragging_ == Drag::None) return false;
    dragging_ = Drag::None;
    return true;
}

void ScrollView::append_to(graphics::DrawList& list) const
{
    if (!content_) return;

    // 内容只画在可滚动区域内，滚动条区域留白。
    list.push_clip({global_.pos.x, global_.pos.y, viewport_.x, viewport_.y});
    content_->append_to(list);
    list.pop_clip();

    const auto& t = ui_theme();
    for (bool vertical : {true, false}) {
        if (!scrollbar_visible(vertical)) continue;

        const Rect track = scrollbar_track_rect(vertical);
        const Rect thumb = scrollbar_thumb_rect(vertical);

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        graphics::Rectangle track_rect{track.pos.x, track.pos.y, track.size.x, track.size.y, t.scrollbar_track};
        track_rect.append_to(list);
        graphics::Rectangle thumb_rect{thumb.pos.x, thumb.pos.y, thumb.size.x, thumb.size.y, t.scrollbar_thumb};
        thumb_rect.append_to(list);
        list.pop_clip();
    }
}
} // namespace xel::ui
