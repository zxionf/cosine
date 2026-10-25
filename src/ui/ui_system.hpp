#pragma once

#include "root.hpp"
#include "../events/event.hpp"

namespace xel::backend { class Window; }

#include <memory>
#include <vector>

namespace xel::ui
{
class UISystem
{
public:
    explicit UISystem(backend::Window& window);
    // 无窗口构造，供纯逻辑测试使用；root 的 global bounds 由构造函数按尺寸设置。
    explicit UISystem(std::unique_ptr<UIRoot> root);

    UIRoot* root() { return root_.get(); }
    UIContext* context() { return root_ ? root_->context() : nullptr; }
    void set_theme(const Theme& theme) { if (root_) root_->set_theme(theme); }
    void tick();

    void dispatch(const event::InputEvent& event);

    Widget* focused() const { return focused_.widget; }
    Widget* hovered() const { return hovered_.widget; }
    void set_focus(Widget* widget);
    void focus_next(bool backwards = false);

private:
    // 交互目标：同时记录指针与生命周期令牌。
    // 控件可能被容器直接销毁（clear_children / set_content），此时令牌过期，
    // 指针即使非空也不能解引用。
    struct Target
    {
        Widget* widget = nullptr;
        std::weak_ptr<const bool> token;

        void set(Widget* w)
        {
            widget = w;
            token = w ? w->alive_token() : std::weak_ptr<const bool>{};
        }
        void clear() { widget = nullptr; token.reset(); }
        bool valid() const { return widget != nullptr && !token.expired(); }

        Target& operator=(Widget* w) { set(w); return *this; }
        Target& operator=(std::nullptr_t) { clear(); return *this; }

        operator Widget*() const { return widget; }
        Widget* operator->() const { return widget; }
        explicit operator bool() const { return valid(); }
    };

    std::vector<Widget*> hit_path(Widget* root, glm::vec2 pos) const;
    void collect_focusable(Widget* widget, std::vector<Widget*>& out) const;
    // 移除 overlay / 子项后清理已失效的 hover、press、focus 目标。
    void prune_stale_targets();

    void dispatch_mouse_move(const event::InputEvent& event);
    void dispatch_mouse_button(const event::InputEvent& event);
    void dispatch_mouse_scroll(const event::InputEvent& event);
    void dispatch_key(const event::InputEvent& event);
    void dispatch_char(const event::InputEvent& event);

    std::unique_ptr<UIRoot> root_;

    Target hovered_;
    Target pressed_;
    Target focused_;

    uint32_t last_width_ = 0;
    uint32_t last_height_ = 0;

    backend::Window* window_ = nullptr;
};
} // namespace xel::ui
