#include "ui_system.hpp"

namespace xel::ui
{
UISystem::UISystem()
{
    root_ = std::make_unique<UIRoot>(100, 100, 1000, 600);
    root_->set_global_bounds(Rect{{100, 100}, {1000, 600}});
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
} // namespace xel::ui