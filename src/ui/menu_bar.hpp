#pragma once

#include "root.hpp"
#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/line.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
class MenuBar;

// 菜单项：普通项、分隔线（label 为空）、快捷键提示。
struct MenuItem
{
    std::string label;
    std::string shortcut;
    bool separator = false;
    bool enabled = true;
    std::function<void()> action;

    static MenuItem separator_item()
    {
        MenuItem item;
        item.separator = true;
        return item;
    }

    static MenuItem make(std::string label, std::function<void()> action,
                         std::string shortcut = {}, bool enabled = true)
    {
        MenuItem item;
        item.label = std::move(label);
        item.shortcut = std::move(shortcut);
        item.action = std::move(action);
        item.enabled = enabled;
        return item;
    }
};

namespace detail
{
// 全屏遮罩：点击空白关闭菜单；鼠标移动到菜单栏区域时切换当前菜单。
class MenuScrim : public Widget
{
public:
    explicit MenuScrim(std::function<void()> dismiss, std::function<void(glm::vec2)> hover_bar)
        : dismiss_{std::move(dismiss)}, hover_bar_{std::move(hover_bar)}
    {}

    glm::vec2 measure(glm::vec2 size) override { return size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList& list) const override
    {
        graphics::Rectangle backdrop{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y,
                                     glm::vec4{0.0f, 0.0f, 0.0f, 0.05f}};
        backdrop.append_to(list);
    }
    bool hit_testable() const override { return true; }
    bool on_mouse_down(glm::vec2, int) override
    {
        if (dismiss_) dismiss_();
        return true;
    }
    bool on_click(glm::vec2, int) override { return true; }
    bool on_scroll(glm::vec2, glm::vec2) override
    {
        if (dismiss_) dismiss_();
        return true;
    }
    bool on_mouse_move(glm::vec2 pos) override
    {
        if (hover_bar_) hover_bar_(pos);
        return true;
    }

private:
    std::function<void()> dismiss_;
    std::function<void(glm::vec2)> hover_bar_;
};

// 下拉菜单浮层：绘制菜单项、处理悬停与点击。
class MenuPopup : public Widget
{
public:
    MenuPopup(MenuBar& owner, int menu_index, float width, float height)
        : owner_{&owner}, menu_index_{menu_index}
    {
        set_min_size({width, height});
        set_max_size({width, height});
    }

    glm::vec2 measure(glm::vec2) override { return local_.size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList& list) const override;
    bool on_mouse_move(glm::vec2 pos) override;
    void on_mouse_leave() override { hovered_ = -1; }
    bool on_click(glm::vec2 pos, int) override;
    bool hit_testable() const override { return true; }

    int hovered() const { return hovered_; }
    void set_hovered(int index) { hovered_ = index; }
    int menu_index() const { return menu_index_; }

    float row_height() const;
    float separator_height() const;

private:
    int index_at(glm::vec2 pos) const;
    MenuBar* owner_ = nullptr;
    int menu_index_ = 0;
    int hovered_ = -1;
};

const std::vector<MenuItem>& menu_items(const MenuBar& bar, int index);
} // namespace detail

// 菜单栏：点击标题展开 overlay 菜单；打开后可用方向键在菜单间移动。
class MenuBar : public Widget
{
public:
    MenuBar() = default;

    int add_menu(std::string title, std::vector<MenuItem> items)
    {
        titles_.push_back(std::move(title));
        menus_.push_back(std::move(items));
        mark_layout_dirty();
        return static_cast<int>(titles_.size()) - 1;
    }

    int menu_count() const { return static_cast<int>(titles_.size()); }
    const std::string& title(int index) const
    {
        static const std::string empty;
        return (index >= 0 && index < menu_count()) ? titles_[static_cast<std::size_t>(index)] : empty;
    }
    const std::vector<MenuItem>& items(int index) const
    {
        static const std::vector<MenuItem> empty;
        return (index >= 0 && index < menu_count()) ? menus_[static_cast<std::size_t>(index)] : empty;
    }
    const std::vector<std::string>& titles() const { return titles_; }
    std::vector<std::vector<MenuItem>>& menus() { return menus_; }

    bool is_open() const { return open_ >= 0; }
    int open_menu() const { return open_; }

    void open(int index)
    {
        if (index < 0 || index >= menu_count()) return;
        if (!is_open() && !create_overlay(index)) return;
        open_ = index;
        highlight_ = first_actionable(index);
        update_popup_geometry();
        apply_hover_highlight();
        mark_layout_dirty();
    }

    void close()
    {
        if (!is_open() && !popup_ && !scrim_) return;
        if (auto* root = find_root()) {
            root->remove_overlay(popup_);
            root->remove_overlay(scrim_);
        }
        popup_ = nullptr;
        scrim_ = nullptr;
        open_ = -1;
        highlight_ = -1;
        mark_layout_dirty();
    }

    void toggle(int index)
    {
        if (is_open() && open_ == index) close();
        else open(index);
    }

    // 由遮罩转发：鼠标在菜单栏区域内移动时切换菜单（经典菜单栏行为）。
    void hover_titles(glm::vec2 pos)
    {
        if (!is_open()) return;
        const int index = title_index_at(pos);
        if (index >= 0 && index != open_) open(index);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        float width = 0.0f;
        for (const auto& title : titles_) width += title_width(title) + t.spacing;
        if (!titles_.empty()) width -= t.spacing;
        return {width, t.control_height};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        if (is_open()) update_popup_geometry();
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        float x = global_.pos.x;
        for (std::size_t i = 0; i < titles_.size(); ++i) {
            const float width = title_width(titles_[i]);
            const bool active = static_cast<int>(i) == open_;
            const bool hovered = static_cast<int>(i) == hovered_ && !active;

            if (active || hovered) {
                graphics::Rectangle background{x, global_.pos.y, width, global_.size.y,
                                               active ? t.button_pressed : t.button_hover};
                background.append_to(list);
            }

            if (auto* atlas = font_atlas()) {
                const glm::vec2 size = atlas->measure(titles_[i], t.text_size);
                atlas->append_text(list, x + (width - size.x) * 0.5f,
                                   global_.pos.y + (global_.size.y - size.y) * 0.5f,
                                   t.text_size, titles_[i], t.text);
            }
            x += width + t.spacing;
        }

        if (focused_) {
            const float b = t.border;
            graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b,
                                       global_.size.x, b, t.focus_ring};
            bottom.append_to(list);
        }
    }

    bool focusable() const override { return true; }
    bool hit_testable() const override { return true; }

    void on_mouse_enter() override { hovered_ = -1; }
    void on_mouse_leave() override { hovered_ = -1; }
    void on_hover(glm::vec2 pos) override
    {
        hovered_ = title_index_at(pos);
        if (is_open() && hovered_ >= 0 && hovered_ != open_) open(hovered_);
    }

    bool on_click(glm::vec2 pos, int) override
    {
        const int index = title_index_at(pos);
        if (index < 0) return false;
        current_ = index;
        toggle(index);
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (menu_count() == 0) return false;

        if (!is_open()) {
            switch (key) {
                case event::KeyCode::Down:
                case event::KeyCode::Enter:
                case event::KeyCode::Space:
                    current_ = std::clamp(current_, 0, menu_count() - 1);
                    open(current_);
                    return true;
                case event::KeyCode::Left:
                    current_ = (current_ - 1 + menu_count()) % menu_count();
                    return true;
                case event::KeyCode::Right:
                    current_ = (current_ + 1) % menu_count();
                    return true;
                default:
                    return false;
            }
        }

        switch (key) {
            case event::KeyCode::Escape:
                close();
                return true;
            case event::KeyCode::Left:
                current_ = (open_ - 1 + menu_count()) % menu_count();
                open(current_);
                return true;
            case event::KeyCode::Right:
                current_ = (open_ + 1) % menu_count();
                open(current_);
                return true;
            case event::KeyCode::Down:
                move_highlight(1);
                return true;
            case event::KeyCode::Up:
                move_highlight(-1);
                return true;
            case event::KeyCode::Home:
                highlight_ = first_actionable(open_);
                apply_hover_highlight();
                return true;
            case event::KeyCode::End:
                highlight_ = last_actionable(open_);
                apply_hover_highlight();
                return true;
            case event::KeyCode::Enter:
            case event::KeyCode::Space:
                activate(highlight_);
                return true;
            default:
                return false;
        }
    }

    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override
    {
        focused_ = false;
        hovered_ = -1;
    }

private:
    friend class detail::MenuPopup;
    friend const std::vector<MenuItem>& detail::menu_items(const MenuBar& bar, int index);

    UIRoot* find_root() const
    {
        for (Widget* widget = const_cast<MenuBar*>(this); widget != nullptr; widget = widget->parent()) {
            if (auto* root = dynamic_cast<UIRoot*>(widget)) return root;
        }
        return nullptr;
    }

    float text_width(const std::string& text) const
    {
        const auto& t = ui_theme();
        if (auto* atlas = font_atlas()) return atlas->measure(text, t.text_size).x;
        return static_cast<float>(text.size()) * t.text_size * 0.5f;
    }

    float title_width(const std::string& title) const
    {
        return text_width(title) + ui_theme().padding * 2.0f;
    }

    int title_index_at(glm::vec2 pos) const
    {
        if (pos.y < global_.pos.y || pos.y > global_.pos.y + global_.size.y) return -1;
        const auto& t = ui_theme();
        float x = global_.pos.x;
        for (std::size_t i = 0; i < titles_.size(); ++i) {
            const float width = title_width(titles_[i]);
            if (pos.x >= x && pos.x <= x + width) return static_cast<int>(i);
            x += width + t.spacing;
        }
        return -1;
    }

    float title_left(int index) const
    {
        const auto& t = ui_theme();
        float x = global_.pos.x;
        for (int i = 0; i < index && i < menu_count(); ++i) {
            x += title_width(titles_[static_cast<std::size_t>(i)]) + t.spacing;
        }
        return x;
    }

    float popup_width(int index) const
    {
        const auto& t = ui_theme();
        float width = 160.0f;
        for (const auto& item : menus_[static_cast<std::size_t>(index)]) {
            if (item.separator) continue;
            const float shortcut_w = item.shortcut.empty()
                ? 0.0f : text_width(item.shortcut) + t.padding * 2.0f;
            width = std::max(width, text_width(item.label) + shortcut_w + t.padding * 4.0f);
        }
        return width;
    }

    float popup_height(int index) const
    {
        const auto& t = ui_theme();
        float height = t.padding;
        for (const auto& item : menus_[static_cast<std::size_t>(index)]) {
            height += item.separator ? 9.0f : t.control_height;
        }
        return height;
    }

    bool create_overlay(int index)
    {
        auto* root = find_root();
        if (!root) return false;

        auto scrim = std::make_unique<detail::MenuScrim>(
            [this]() { close(); },
            [this](glm::vec2 pos) { hover_titles(pos); });
        scrim_ = scrim.get();

        auto popup = std::make_unique<detail::MenuPopup>(
            *this, index, popup_width(index), popup_height(index));
        popup_ = popup.get();

        root->add_overlay(std::move(scrim));
        root->add_overlay(std::move(popup));
        return true;
    }

    void update_popup_geometry()
    {
        if (!popup_ || open_ < 0) return;
        const auto& t = ui_theme();
        const float width = popup_width(open_);
        const float height = popup_height(open_);
        const float x = title_left(open_);
        const float y = global_.pos.y + global_.size.y + 2.0f;
        popup_->set_min_size({width, height});
        popup_->set_max_size({width, height});
        popup_->set_pos(x, y);
        popup_->set_size(width, height);
        popup_->arrange({{x, y}, {width, height}});
        (void)t;
    }

    int first_actionable(int index) const
    {
        const auto& list = menus_[static_cast<std::size_t>(index)];
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (!list[i].separator && list[i].enabled) return static_cast<int>(i);
        }
        return -1;
    }

    int last_actionable(int index) const
    {
        const auto& list = menus_[static_cast<std::size_t>(index)];
        for (int i = static_cast<int>(list.size()) - 1; i >= 0; --i) {
            if (!list[static_cast<std::size_t>(i)].separator && list[static_cast<std::size_t>(i)].enabled) {
                return i;
            }
        }
        return -1;
    }

    void move_highlight(int delta)
    {
        if (open_ < 0) return;
        const auto& list = menus_[static_cast<std::size_t>(open_)];
        if (list.empty()) return;
        int index = highlight_;
        for (int step = 0; step < static_cast<int>(list.size()); ++step) {
            index += delta;
            if (index < 0) index = static_cast<int>(list.size()) - 1;
            if (index >= static_cast<int>(list.size())) index = 0;
            const auto& item = list[static_cast<std::size_t>(index)];
            if (!item.separator && item.enabled) {
                highlight_ = index;
                apply_hover_highlight();
                return;
            }
        }
    }

    void apply_hover_highlight()
    {
        if (popup_) popup_->set_hovered(highlight_);
    }

    void activate(int index)
    {
        if (open_ < 0 || index < 0) return;
        const auto& list = menus_[static_cast<std::size_t>(open_)];
        if (index >= static_cast<int>(list.size())) return;
        const auto& item = list[static_cast<std::size_t>(index)];
        if (item.separator || !item.enabled) return;

        // 先关闭再执行回调：回调里可以安全地打开对话框或修改菜单结构。
        auto action = item.action;
        close();
        if (action) action();
    }

    std::vector<std::string> titles_;
    std::vector<std::vector<MenuItem>> menus_;
    int open_ = -1;
    int highlight_ = -1;
    int hovered_ = -1;
    int current_ = 0;
    bool focused_ = false;
    detail::MenuPopup* popup_ = nullptr;
    detail::MenuScrim* scrim_ = nullptr;
};

namespace detail
{
inline const std::vector<MenuItem>& menu_items(const MenuBar& bar, int index)
{
    static const std::vector<MenuItem> empty;
    if (index < 0 || index >= bar.menu_count()) return empty;
    return bar.menus_[static_cast<std::size_t>(index)];
}

inline float MenuPopup::row_height() const
{
    return owner_->ui_theme().control_height;
}

inline float MenuPopup::separator_height() const
{
    return 9.0f;
}

inline int MenuPopup::index_at(glm::vec2 pos) const
{
    if (!global_.contains(pos)) return -1;
    float y = global_.pos.y + owner_->ui_theme().padding;
    const auto& list = menu_items(*owner_, menu_index_);
    for (std::size_t i = 0; i < list.size(); ++i) {
        const float row = list[i].separator ? separator_height() : row_height();
        if (pos.y >= y && pos.y <= y + row) return list[i].separator ? -1 : static_cast<int>(i);
        y += row;
    }
    return -1;
}

inline void MenuPopup::append_to(graphics::DrawList& list) const
{
    const auto& t = owner_->ui_theme();

    graphics::Rectangle background{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y,
                                   t.panel_bg};
    background.append_to(list);

    const float b = t.border;
    graphics::Rectangle top{global_.pos.x, global_.pos.y, global_.size.x, b, t.panel_border};
    graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b,
                               global_.size.x, b, t.panel_border};
    graphics::Rectangle left{global_.pos.x, global_.pos.y, b, global_.size.y, t.panel_border};
    graphics::Rectangle right{global_.pos.x + global_.size.x - b, global_.pos.y, b,
                              global_.size.y, t.panel_border};
    top.append_to(list);
    bottom.append_to(list);
    left.append_to(list);
    right.append_to(list);

    auto* atlas = owner_->font_atlas();
    float y = global_.pos.y + t.padding;
    const auto& items = menu_items(*owner_, menu_index_);

    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        const float row = item.separator ? separator_height() : row_height();

        if (item.separator) {
            graphics::Rectangle line{global_.pos.x + t.padding, y + row * 0.5f,
                                     global_.size.x - t.padding * 2.0f, 1.0f, t.panel_border};
            line.append_to(list);
            y += row;
            continue;
        }

        const bool active = static_cast<int>(i) == hovered_ && item.enabled;
        if (active) {
            graphics::Rectangle highlight{global_.pos.x + 2.0f, y,
                                          global_.size.x - 4.0f, row, t.button_hover};
            highlight.append_to(list);
        }

        const glm::vec4 color = item.enabled ? t.text : t.text_disabled;
        if (atlas) {
            const float text_y = y + (row - t.text_size) * 0.5f;
            atlas->append_text(list, global_.pos.x + t.padding * 2.0f, text_y,
                               t.text_size, item.label, color);
            if (!item.shortcut.empty()) {
                const float shortcut_w = atlas->measure(item.shortcut, t.text_size).x;
                atlas->append_text(list,
                                   global_.pos.x + global_.size.x - t.padding * 2.0f - shortcut_w,
                                   text_y, t.text_size, item.shortcut, t.text_disabled);
            }
        }
        y += row;
    }
}

inline bool MenuPopup::on_mouse_move(glm::vec2 pos)
{
    hovered_ = index_at(pos);
    if (hovered_ >= 0) owner_->highlight_ = hovered_;
    return true;
}

inline bool MenuPopup::on_click(glm::vec2 pos, int)
{
    const int index = index_at(pos);
    if (index < 0) return true;
    owner_->activate(index);
    return true;
}
} // namespace detail
} // namespace xel::ui
