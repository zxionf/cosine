#pragma once

#include "ui/composites/menu_item.hpp"
#include "ui/node/widget.hpp"

#include <functional>
#include <vector>

namespace xel::ui
{
class MenuBar;

namespace detail
{
// 全屏遮罩：点击空白关闭菜单；
// 鼠标移动到菜单栏区域时切换当前菜单。
class MenuScrim : public Widget
{
public:
    explicit MenuScrim(std::function<void()> dismiss, std::function<void(glm::vec2)> hover_bar)
        : dismiss_{std::move(dismiss)}, hover_bar_{std::move(hover_bar)}
    {}

    glm::vec2 measure(glm::vec2 size) override { return size; }
    void arrange(Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList& list) const override;
    bool hit_testable() const override { return true; }
    Layer layer() const override { return Layer::Floating; }
    bool on_mouse_down(glm::vec2, int) override;
    bool on_click(glm::vec2, int) override { return true; }
    bool on_scroll(glm::vec2, glm::vec2) override;
    bool on_mouse_move(glm::vec2 pos) override;

private:
    std::function<void()> dismiss_;
    std::function<void(glm::vec2)> hover_bar_;
};

// 下拉菜单浮层：绘制菜单项、处理悬停与点击；可持有任意层子菜单的内容。
class MenuPopup : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::Menu; }
    MenuPopup(MenuBar& owner, float width, float height);

    glm::vec2 measure(glm::vec2) override { return clamp_size(preferred_size()); }
    // 宿主布局时每帧都会重新定位弹层：只写几何、不标脏。
    void arrange(Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList& list) const override;
    bool on_mouse_move(glm::vec2 pos) override;
    void on_mouse_leave() override { hovered_ = -1; }
    bool on_click(glm::vec2 pos, int) override;
    bool hit_testable() const override { return true; }
    Layer layer() const override { return Layer::Floating; }

    // 内容源：顶层菜单或某层子菜单的拷贝（菜单项数量少，拷贝换来地址稳定）。
    const std::vector<MenuItem>& items() const { return items_; }
    void set_items(std::vector<MenuItem> items);

    int hovered() const { return hovered_; }
    void set_hovered(int index)
    {
        hovered_ = index;
        if (index >= 0) keyboard_highlight_ = -1;
    }
    int keyboard_highlight() const { return keyboard_highlight_; }
    void set_keyboard_highlight(int index)
    {
        keyboard_highlight_ = index;
        if (index >= 0) hovered_ = -1;
    }
    // 绘制用：鼠标优先，其次键盘。
    int highlighted() const { return hovered_ >= 0 ? hovered_ : keyboard_highlight_; }

    float row_height() const;
    float separator_height() const;
    // 某一项的顶部 y（global），供子菜单定位。
    float item_top(int index) const;
    float content_width() const;
    float content_height() const;

private:
    int index_at(glm::vec2 pos) const;
    MenuBar* owner_ = nullptr;
    std::vector<MenuItem> items_;
    int hovered_ = -1;
    int keyboard_highlight_ = -1;
};

const std::vector<MenuItem>& menu_items(const MenuBar& bar, int index);
} // namespace detail
} // namespace xel::ui
