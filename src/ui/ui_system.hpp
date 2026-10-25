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
    void tick();

    void dispatch(const event::InputEvent& event);

    Widget* focused() const { return focused_; }
    Widget* hovered() const { return hovered_; }
    void set_focus(Widget* widget);
    void focus_next(bool backwards = false);

private:
    std::vector<Widget*> hit_path(Widget* root, glm::vec2 pos) const;
    void collect_focusable(Widget* widget, std::vector<Widget*>& out) const;

    void dispatch_mouse_move(const event::InputEvent& event);
    void dispatch_mouse_button(const event::InputEvent& event);
    void dispatch_mouse_scroll(const event::InputEvent& event);
    void dispatch_key(const event::InputEvent& event);
    void dispatch_char(const event::InputEvent& event);

    std::unique_ptr<UIRoot> root_;

    Widget* hovered_ = nullptr;
    Widget* pressed_ = nullptr;
    Widget* focused_ = nullptr;

    uint32_t last_width_ = 0;
    uint32_t last_height_ = 0;

    backend::Window* window_ = nullptr;
};
} // namespace xel::ui
