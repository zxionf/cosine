#include "ui_system.hpp"

namespace xel::ui
{
UISystem::UISystem(backend::Window& window) : window_{window}
{
    root_ = std::make_unique<UIRoot>(0, 0, 0, 0);
}

void UISystem::tick()
{
    const auto extent = window_.get_extent();
    const bool resized = extent.width != last_width_ || extent.height != last_height_;

    if (resized) {
        last_width_ = extent.width;
        last_height_ = extent.height;
        root_->set_pos(0, 0);
        root_->set_size(extent.width, extent.height);
        root_->set_global_bounds({{0, 0}, {static_cast<float>(extent.width), static_cast<float>(extent.height)}});
        root_->mark_layout_dirty();
    }

    if (resized || root_->layout_dirty()) {
        root_->layout();
        root_->clear_layout_dirty();
    }
}

Widget* UISystem::deep_hit(Widget* widget, glm::vec2 pos)
{
    if (!widget || !widget->visible()) return nullptr;

    // 容器可以裁剪子控件：超出范围的内容不可命中
    if (widget->clips_children() && !widget->global_bounds().contains(pos)) return nullptr;

    // 顶层优先命中
    const auto& children = widget->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (auto* hit = deep_hit(it->get(), pos)) return hit;
    }
    // 只在自己可命中时才返回自己
    if (widget->hit_testable() && widget->global_bounds().contains(pos))
        return widget;
    return nullptr;
}

void UISystem::dispatch(const event::InputEvent& event)
{
    switch (event.type) {
        case event::InputEvent::Type::MouseMove: dispatch_mouse_move(event); break;
        case event::InputEvent::Type::MouseButtonDown:
        case event::InputEvent::Type::MouseButtonUp: dispatch_mouse_button(event); break;
        case event::InputEvent::Type::MouseScroll: dispatch_mouse_scroll(event); break;
        default: break;
    }
}

void UISystem::dispatch_mouse_move(const event::InputEvent& event)
{
    // 拖动时：优先发给 pressed_
    if (pressed_) {
        pressed_->on_mouse_move(event.mouse_pos);
        return;
    }
    Widget* target = deep_hit(root_.get(), event.mouse_pos);
    // 更新 hovered
    if (target != hovered_) {
        if (hovered_) hovered_->on_mouse_leave();
        hovered_ = target;
        if (hovered_) hovered_->on_mouse_enter();
    }
    if (hovered_) hovered_->on_mouse_move(event.mouse_pos);
}

void UISystem::dispatch_mouse_button(const event::InputEvent& event)
{
    Widget* target = deep_hit(root_.get(), event.mouse_pos);

    if (event.type == event::InputEvent::Type::MouseButtonDown) {
        pressed_ = target;
        if (pressed_) pressed_->on_mouse_down(event.mouse_pos, event.mouse_button);
        return;
    }

    if (pressed_ && pressed_ == target) {
        pressed_->on_click(event.mouse_pos, event.mouse_button);
    }
    if (target) target->on_mouse_up(event.mouse_pos, event.mouse_button);
    pressed_ = nullptr;
}

void UISystem::dispatch_mouse_scroll(const event::InputEvent& event)
{
    if (hovered_ && hovered_->on_scroll(event.mouse_pos, event.scroll_delta)) return;
    if (pressed_ && pressed_ != hovered_) pressed_->on_scroll(event.mouse_pos, event.scroll_delta);
}
} // namespace xel::ui
