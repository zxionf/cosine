#include "ui_system.hpp"

namespace xel::ui
{
UISystem::UISystem(backend::Window& window) : window_{window}
{
    root_ = std::make_unique<UIRoot>(100, 100, 1000, 600);
    root_->set_global_bounds(Rect{{100, 100}, {1000, 600}});
}

void UISystem::tick()
{
    root_->set_pos(0, 0);
    root_->set_size(window_.get_extent().width, window_.get_extent().height);
    root_->set_global_bounds({{0,0},{window_.get_extent().width, window_.get_extent().height}});
    root_->layout();
}

Widget* UISystem::deep_hit(Widget* widget, glm::vec2 pos)
{
    if (!widget->visible()) return nullptr;
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
        case event::InputEvent::Type::MouseScroll: dispatch_mouse_scroll(event); break;
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

void UISystem::dispatch_mouse_scroll(const event::InputEvent& event)
{
    if (hovered_) hovered_->on_scroll(event.mouse_pos, event.scroll_delta);
    if (pressed_) pressed_->on_scroll(event.mouse_pos, event.scroll_delta);
}
} // namespace xel::ui