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

    // overlay 层优先命中，其次才是普通子项；两者内部按 z-index 降序（大的先命中）。
    const std::vector<xel::ui::Widget*> overlays = widget->overlay_paint_order();
    for (auto it = overlays.rbegin(); it != overlays.rend(); ++it) {
        if (collect_hit_path(*it, pos, path)) return true;
    }

    // 顶层优先命中
    const std::vector<xel::ui::Widget*> children = widget->paint_order();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (collect_hit_path(*it, pos, path)) return true;
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
    tick(1.0f / 60.0f);
}

void UISystem::tick(float dt)
{
    // 先清理悬垂目标，再触发 layout（layout 会销毁已移除的 overlay）。
    prune_stale_targets();

    // 指针手势的时间基准：用自身 dt 累加，不依赖系统时钟。
    elapsed_ += dt;
    update_long_press();

    bool resized = false;
    if (window_) {
        const auto extent = window_->get_extent();
        resized = extent.width != last_width_ || extent.height != last_height_;
        if (resized) {
            last_width_ = extent.width;
            last_height_ = extent.height;
            root_->set_pos(0, 0);
            // 物理 extent → 逻辑尺寸；投影与输入按同一比例换算。
            const float logical_w = static_cast<float>(extent.width) / ui_scale_;
            const float logical_h = static_cast<float>(extent.height) / ui_scale_;
            root_->set_size(logical_w, logical_h);
            root_->set_global_bounds({{0, 0}, {logical_w, logical_h}});
            root_->mark_layout_dirty();
        }
    }

    if (resized || root_->layout_dirty()) {
        root_->layout();
        root_->clear_layout_dirty();
    }

    // 布局完成后再推进动画：过渡只改颜色，不会触发重排。
    update_tree(root_.get(), dt);
}

void UISystem::update_tree(Widget* widget, float dt)
{
    if (!widget || !widget->visible()) return;
    widget->update(dt);
    for (xel::ui::Widget* child : widget->paint_order()) {
        update_tree(child, dt);
    }
    for (xel::ui::Widget* overlay : widget->overlay_paint_order()) {
        update_tree(overlay, dt);
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
    // 记录最新指针位置：长按/双击判定用。
    switch (event.type) {
        case event::InputEvent::Type::MouseMove:
        case event::InputEvent::Type::MouseButtonDown:
        case event::InputEvent::Type::MouseButtonUp:
            pointer_pos_ = event.mouse_pos;
            break;
        case event::InputEvent::Type::MouseScroll:
        case event::InputEvent::Type::KeyDown:
        case event::InputEvent::Type::KeyUp:
        case event::InputEvent::Type::CharInput:
            break;
    }

    // 输入来源决定焦点环是否可见：键盘操作期间显示，鼠标操作期间隐藏。
    if (auto* ctx = context()) {
        switch (event.type) {
            case event::InputEvent::Type::KeyDown:
            case event::InputEvent::Type::KeyUp:
                ctx->input_mode = InputMode::Keyboard;
                break;
            case event::InputEvent::Type::MouseMove:
            case event::InputEvent::Type::MouseButtonDown:
            case event::InputEvent::Type::MouseButtonUp:
                ctx->input_mode = InputMode::Mouse;
                break;
            case event::InputEvent::Type::MouseScroll:
            case event::InputEvent::Type::CharInput:
                break;
        }
    }

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
    // overlay 处于顶层，Tab 顺序也优先进入；同层按 z-index 升序。
    for (xel::ui::Widget* overlay : widget->overlay_paint_order()) {
        collect_focusable(overlay, out);
    }
    for (xel::ui::Widget* child : widget->paint_order()) {
        collect_focusable(child, out);
    }
}

void UISystem::focus_next(bool backwards)
{
    // 先按树序收集全部可聚焦控件（含 tab_order < 0 的），用于定位起点。
    std::vector<Widget*> ordered;
    collect_focusable(root_.get(), ordered);
    if (ordered.empty()) return;

    struct Entry
    {
        Widget* widget = nullptr;
        std::size_t tree_index = 0;
    };

    std::vector<Entry> tabbable;
    tabbable.reserve(ordered.size());
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        if (ordered[i]->tab_order() >= 0) tabbable.push_back({ordered[i], i});
    }
    if (tabbable.empty()) return;

    // 显式 order 整体排在「按树序」之前；同值或未设时保持树序。
    std::stable_sort(tabbable.begin(), tabbable.end(), [](const Entry& a, const Entry& b) {
        const int ao = a.widget->tab_order() > 0 ? a.widget->tab_order() : kAutoTabOrder;
        const int bo = b.widget->tab_order() > 0 ? b.widget->tab_order() : kAutoTabOrder;
        return ao < bo;
    });

    auto position_of = [&](Widget* widget) -> std::size_t {
        for (std::size_t i = 0; i < tabbable.size(); ++i) {
            if (tabbable[i].widget == widget) return i;
        }
        return tabbable.size(); // 不在 Tab 序列中
    };

    std::size_t index = 0;
    const std::size_t current = position_of(focused_.widget);
    if (current < tabbable.size()) {
        index = backwards ? (current + tabbable.size() - 1) % tabbable.size()
                          : (current + 1) % tabbable.size();
    } else if (focused_.widget != nullptr) {
        // 焦点在 tab_order < 0 的控件上（如 RadioGroup 内被点击的 Radio）：
        // 按树序找其后第一个可 Tab 的目标，循环回绕。
        std::size_t start = ordered.size();
        for (std::size_t i = 0; i < ordered.size(); ++i) {
            if (ordered[i] == focused_.widget) {
                start = i;
                break;
            }
        }

        if (start == ordered.size()) {
            index = backwards ? tabbable.size() - 1 : 0;
        } else {
            std::size_t best = tabbable.size();
            for (std::size_t i = 0; i < tabbable.size(); ++i) {
                const std::size_t tree_index = tabbable[i].tree_index;
                const bool candidate = backwards ? tree_index < start : tree_index > start;
                if (!candidate) continue;
                if (best == tabbable.size() ||
                    (backwards ? tree_index > tabbable[best].tree_index
                               : tree_index < tabbable[best].tree_index)) {
                    best = i;
                }
            }
            if (best == tabbable.size()) {
                index = backwards ? tabbable.size() - 1 : 0;
            } else {
                index = best;
            }
        }
    } else {
        index = backwards ? tabbable.size() - 1 : 0;
    }

    set_focus(tabbable[index].widget);
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
    const bool is_down = event.type == event::InputEvent::Type::MouseButtonDown;
    const int button = event.mouse_button;

    const auto path = hit_path(root_.get(), event.mouse_pos);
    Widget* target = path.empty() ? nullptr : path.back();

    if (is_down) {
        pressed_ = target;
        if (pressed_) {
            // 只有左键抢焦点：右键不应改变焦点，否则右键菜单与焦点关系无法推理。
            if (button == kButtonLeft && pressed_->focusable()) set_focus(pressed_);
            press_ = PressRecord{true, event.mouse_pos, button, elapsed_, false};
            bubble_to_root(pressed_, [&](Widget* widget) {
                return widget->on_mouse_down(event.mouse_pos, button);
            });
        }
        return;
    }

    Widget* pressed = pressed_;
    pressed_ = nullptr;
    const bool long_fired = press_.long_fired;
    press_ = PressRecord{};

    if (!pressed) {
        if (target) {
            bubble_to_root(target, [&](Widget* widget) {
                return widget->on_mouse_up(event.mouse_pos, button);
            });
        }
        return;
    }

    bubble_to_root(pressed, [&](Widget* widget) {
        return widget->on_mouse_up(event.mouse_pos, button);
    });

    if (pressed != target) return;

    // 右键：只发 on_right_click，不触发 on_click，也不参与双击。
    if (button == kButtonRight) {
        bubble_to_root(pressed, [&](Widget* widget) {
            return widget->on_right_click(event.mouse_pos);
        });
        return;
    }

    bubble_to_root(pressed, [&](Widget* widget) {
        return widget->on_click(event.mouse_pos, button);
    });

    // 双击判定：同目标 + 同按钮 + 时间/位置都在阈值内。
    const bool same_target = last_click_.valid && last_click_.target.valid() &&
                             last_click_.target.widget == pressed;
    const bool within_time = same_target && double_click_ms_ > 0.0f &&
                             (elapsed_ - last_click_.time) * 1000.0f <= double_click_ms_;
    const bool within_slop = within_time &&
                             glm::length(event.mouse_pos - last_click_.pos) <= double_click_slop_;
    const bool same_button = last_click_.button == button;

    if (same_target && within_time && within_slop && same_button) {
        // 长按已经触发时不再追加双击，避免两套手势同时生效。
        if (!long_fired) {
            bubble_to_root(pressed, [&](Widget* widget) {
                return widget->on_double_click(event.mouse_pos, button);
            });
        }
        last_click_ = ClickRecord{};
        return;
    }

    last_click_ = ClickRecord{};
    last_click_.target = pressed;
    last_click_.pos = event.mouse_pos;
    last_click_.button = button;
    last_click_.time = elapsed_;
    last_click_.valid = true;
}

void UISystem::update_long_press()
{
    if (!press_.active || press_.long_fired) return;
    if (long_press_ms_ <= 0.0f) return;
    if (!pressed_.valid()) {
        press_.active = false;
        return;
    }
    // 指针移出目标就取消，避免拖动时误触。
    if (!pressed_->global_bounds().contains(pointer_pos_)) {
        press_.active = false;
        return;
    }
    if ((elapsed_ - press_.time) * 1000.0f < long_press_ms_) return;

    press_.long_fired = true;
    const glm::vec2 press_pos = press_.pos;
    const int press_button = press_.button;
    bubble_to_root(pressed_, [&](Widget* widget) {
        return widget->on_long_press(press_pos, press_button);
    });
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
