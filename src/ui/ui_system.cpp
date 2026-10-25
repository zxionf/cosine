#include "ui_system.hpp"
#include "../backend/window.hpp"

#include <algorithm>

namespace
{
template <typename Fn>
bool bubble_to_root(xel::ui::Widget* target, Fn&& fn)
{
    for (xel::ui::Widget* widget = target; widget != nullptr; widget = widget->parent()) {
        if (fn(widget)) return true;
    }
    return false;
}

bool collect_hit_path(xel::ui::Widget* widget, glm::vec2 pos, std::vector<xel::ui::Widget*>& path)
{
    if (!widget || !widget->visible()) return false;

    // 容器可以裁剪子控件：超出范围的内容不可命中
    if (widget->clips_children() && !widget->global_bounds().contains(pos)) return false;

    path.push_back(widget);

    // overlay 层优先命中，其次才是普通子项；两者内部都按后加入者优先。
    const auto& overlays = widget->overlays();
    for (auto it = overlays.rbegin(); it != overlays.rend(); ++it) {
        if (collect_hit_path(it->get(), pos, path)) return true;
    }

    // 顶层优先命中
    const auto& children = widget->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (collect_hit_path(it->get(), pos, path)) return true;
    }
    // 只在自己可命中时才返回自己
    if (widget->hit_testable() && widget->global_bounds().contains(pos)) return true;

    path.pop_back();
    return false;
}

bool tree_contains(const xel::ui::Widget* widget, const xel::ui::Widget* target)
{
    if (!widget) return false;
    if (widget == target) return true;
    for (const auto& child : widget->children()) {
        if (tree_contains(child.get(), target)) return true;
    }
    for (const auto& overlay : widget->overlays()) {
        if (tree_contains(overlay.get(), target)) return true;
    }
    return false;
}
} // namespace

namespace xel::ui
{
UISystem::UISystem(backend::Window& window) : window_{&window}
{
    root_ = std::make_unique<UIRoot>(0, 0, 0, 0);
}

UISystem::UISystem(std::unique_ptr<UIRoot> root) : root_{std::move(root)}
{
    if (root_) {
        root_->set_global_bounds({{0.0f, 0.0f}, {root_->width(), root_->height()}});
        root_->mark_layout_dirty();
    }
}

void UISystem::prune_stale_targets()
{
    // 两种失效路径：
    // 1) 控件已被容器直接销毁（令牌过期）——不能再调用任何虚函数；
    // 2) 控件还在，但已从树上移除（如 overlay 被移除，layout 时才销毁）——
    //    先通知 leave/lost，再清理目标。
    auto in_tree = [this](Widget* widget) {
        return widget != nullptr && tree_contains(root_.get(), widget);
    };

    if (hovered_.widget) {
        if (!hovered_.valid()) {
            hovered_.clear();
        } else if (!in_tree(hovered_.widget)) {
            hovered_->set_hovered(false);
            hovered_->on_mouse_leave();
            hovered_.clear();
        }
    }

    if (pressed_.widget && (!pressed_.valid() || !in_tree(pressed_.widget))) {
        pressed_.clear();
    }

    if (focused_.widget) {
        if (!focused_.valid()) {
            focused_.clear();
        } else if (!in_tree(focused_.widget)) {
            focused_->on_focus_lost();
            focused_.clear();
        }
    }
}

void UISystem::tick()
{
    // 先清理悬垂目标，再触发 layout（layout 会销毁已移除的 overlay）。
    prune_stale_targets();

    bool resized = false;
    if (window_) {
        const auto extent = window_->get_extent();
        resized = extent.width != last_width_ || extent.height != last_height_;
        if (resized) {
            last_width_ = extent.width;
            last_height_ = extent.height;
            root_->set_pos(0, 0);
            root_->set_size(extent.width, extent.height);
            root_->set_global_bounds({{0, 0}, {static_cast<float>(extent.width), static_cast<float>(extent.height)}});
            root_->mark_layout_dirty();
        }
    }

    if (resized || root_->layout_dirty()) {
        root_->layout();
        root_->clear_layout_dirty();
    }
}

std::vector<Widget*> UISystem::hit_path(Widget* root, glm::vec2 pos) const
{
    std::vector<Widget*> path;
    collect_hit_path(root, pos, path);
    return path;
}

void UISystem::dispatch(const event::InputEvent& event)
{
    switch (event.type) {
        case event::InputEvent::Type::MouseMove: dispatch_mouse_move(event); break;
        case event::InputEvent::Type::MouseButtonDown:
        case event::InputEvent::Type::MouseButtonUp: dispatch_mouse_button(event); break;
        case event::InputEvent::Type::MouseScroll: dispatch_mouse_scroll(event); break;
        case event::InputEvent::Type::KeyDown:
        case event::InputEvent::Type::KeyUp: dispatch_key(event); break;
        case event::InputEvent::Type::CharInput: dispatch_char(event); break;
    }
}

void UISystem::set_focus(Widget* widget)
{
    if (focused_ == widget) return;
    if (focused_) focused_->on_focus_lost();
    focused_ = widget;
    if (focused_) focused_->on_focus_gained();
}

void UISystem::collect_focusable(Widget* widget, std::vector<Widget*>& out) const
{
    if (!widget || !widget->visible()) return;
    if (widget->focusable()) out.push_back(widget);
    // overlay 处于顶层，Tab 顺序也优先进入。
    for (const auto& overlay : widget->overlays()) {
        collect_focusable(overlay.get(), out);
    }
    for (const auto& child : widget->children()) {
        collect_focusable(child.get(), out);
    }
}

void UISystem::focus_next(bool backwards)
{
    std::vector<Widget*> focusable;
    collect_focusable(root_.get(), focusable);
    if (focusable.empty()) return;

    auto it = std::find(focusable.begin(), focusable.end(), focused_);
    std::size_t index = 0;
    if (it != focusable.end()) {
        index = static_cast<std::size_t>(std::distance(focusable.begin(), it));
        if (backwards) {
            index = (index + focusable.size() - 1) % focusable.size();
        } else {
            index = (index + 1) % focusable.size();
        }
    } else if (backwards) {
        index = focusable.size() - 1;
    }

    set_focus(focusable[index]);
}

void UISystem::dispatch_mouse_move(const event::InputEvent& event)
{
    // 拖动时：优先发给捕获目标
    if (pressed_) {
        bubble_to_root(pressed_, [&](Widget* widget) {
            return widget->on_mouse_move(event.mouse_pos);
        });
        return;
    }

    const auto path = hit_path(root_.get(), event.mouse_pos);
    Widget* target = path.empty() ? nullptr : path.back();

    if (target != hovered_) {
        if (hovered_) {
            hovered_->set_hovered(false);
            hovered_->on_mouse_leave();
        }
        hovered_ = target;
        if (hovered_) {
            hovered_->set_hovered(true);
            hovered_->on_mouse_enter();
        }
    }

    if (hovered_) {
        hovered_->on_hover(event.mouse_pos);
        bubble_to_root(hovered_, [&](Widget* widget) {
            return widget->on_mouse_move(event.mouse_pos);
        });
    }
}

void UISystem::dispatch_mouse_button(const event::InputEvent& event)
{
    const auto path = hit_path(root_.get(), event.mouse_pos);
    Widget* target = path.empty() ? nullptr : path.back();

    if (event.type == event::InputEvent::Type::MouseButtonDown) {
        pressed_ = target;
        if (pressed_) {
            if (pressed_->focusable()) set_focus(pressed_);
            bubble_to_root(pressed_, [&](Widget* widget) {
                return widget->on_mouse_down(event.mouse_pos, event.mouse_button);
            });
        }
        return;
    }

    Widget* pressed = pressed_;
    pressed_ = nullptr;
    if (!pressed) {
        if (target) {
            bubble_to_root(target, [&](Widget* widget) {
                return widget->on_mouse_up(event.mouse_pos, event.mouse_button);
            });
        }
        return;
    }

    bubble_to_root(pressed, [&](Widget* widget) {
        return widget->on_mouse_up(event.mouse_pos, event.mouse_button);
    });

    if (pressed == target) {
        bubble_to_root(pressed, [&](Widget* widget) {
            return widget->on_click(event.mouse_pos, event.mouse_button);
        });
    }
}

void UISystem::dispatch_mouse_scroll(const event::InputEvent& event)
{
    Widget* target = hovered_;
    if (!target) {
        const auto path = hit_path(root_.get(), event.mouse_pos);
        target = path.empty() ? nullptr : path.back();
    }
    if (!target) return;

    bubble_to_root(target, [&](Widget* widget) {
        return widget->on_scroll(event.mouse_pos, {event.scroll_delta_x, event.scroll_delta});
    });
}

void UISystem::dispatch_key(const event::InputEvent& event)
{
    if (event.type == event::InputEvent::Type::KeyDown && event.key == event::KeyCode::Tab) {
        focus_next(event::has_modifier(event.mods, event::Modifier::Shift));
        return;
    }
    if (!focused_) return;

    bubble_to_root(focused_, [&](Widget* widget) {
        return event.type == event::InputEvent::Type::KeyDown
            ? widget->on_key_down(event.key, event.mods)
            : widget->on_key_up(event.key, event.mods);
    });
}

void UISystem::dispatch_char(const event::InputEvent& event)
{
    if (!focused_) return;
    bubble_to_root(focused_, [&](Widget* widget) {
        return widget->on_char(event.codepoint);
    });
}
} // namespace xel::ui
