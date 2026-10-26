#pragma once

#include "root.hpp"
#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "../graphics/rectangle.hpp"
#include "../graphics/triangle.hpp"

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
    // 递归结构：天然支持任意层子菜单。
    std::vector<MenuItem> children;

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

    // 子菜单项：children 非空时显示右向箭头，悬停/Enter 展开。
    static MenuItem submenu(std::string label, std::vector<MenuItem> children, bool enabled = true)
    {
        MenuItem item;
        item.label = std::move(label);
        item.children = std::move(children);
        item.enabled = enabled;
        return item;
    }

    bool has_submenu() const { return !children.empty(); }
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

// 下拉菜单浮层：绘制菜单项、处理悬停与点击；可持有任意层子菜单的内容。
class MenuPopup : public Widget
{
public:
    MenuPopup(MenuBar& owner, float width, float height)
        : owner_{&owner}
    {
        // 尺寸由 arrange() 直接写入 local_；不要设置 min/max_size，
        // 否则 clamp_size 会把后续尺寸锁死在创建时的值上。
        local_ = {{0.0f, 0.0f}, {width, height}};
    }

    glm::vec2 measure(glm::vec2) override { return local_.size; }
    // 直接写 local_/global_：宿主布局时每帧都会重新定位弹层，
    // 走 set_pos/set_size 会持续标脏并触发整树重排。
    void arrange(Rect slot) override
    {
        local_ = slot;
        global_ = slot;
    }
    void append_to(graphics::DrawList& list) const override;
    bool on_mouse_move(glm::vec2 pos) override;
    void on_mouse_leave() override { hovered_ = -1; }
    bool on_click(glm::vec2 pos, int) override;
    bool hit_testable() const override { return true; }

    // 内容源：顶层菜单或某层子菜单的拷贝（菜单项数量少，拷贝换来地址稳定）。
    const std::vector<MenuItem>& items() const { return items_; }
    void set_items(std::vector<MenuItem> items)
    {
        items_ = std::move(items);
        hovered_ = -1;
        keyboard_highlight_ = -1;
        const glm::vec2 size = {content_width(), content_height()};
        local_ = {global_.pos, size};
        global_ = local_;
    }

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
    // 当前打开的子菜单层数（0 = 只有顶层菜单）。
    int submenu_depth() const
    {
        return popups_.empty() ? 0 : static_cast<int>(popups_.size()) - 1;
    }

    void open(int index)
    {
        if (index < 0 || index >= menu_count()) return;
        if (!ensure_overlay()) return;
        open_ = index;
        active_depth_ = 0;
        // 切换菜单时复用同一个 popup widget，只换内容（overlay 生命周期保持不变）。
        close_deeper(1);
        popups_[0]->set_items(menus_[static_cast<std::size_t>(index)]);
        popups_[0]->set_keyboard_highlight(first_actionable_of(popups_[0]->items()));
        update_popup_geometry();
        mark_layout_dirty();
    }

    void close()
    {
        close_all_popups();
        if (auto* root = find_root(); root && scrim_) root->remove_overlay(scrim_);
        scrim_ = nullptr;
        open_ = -1;
        active_depth_ = 0;
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
                const BoxStyle style = t.style(StyleRole::MenuItem,
                                               active ? ControlState::Pressed : ControlState::Hovered);
                graphics::append_rounded_rect(list, {x, global_.pos.y, width, global_.size.y},
                                              corner_radius(style.radius), style.fill);
            }

            if (auto* atlas = font_atlas()) {
                const glm::vec2 size = atlas->measure(titles_[i], t.text_size);
                atlas->append_text(list, x + (width - size.x) * 0.5f,
                                   global_.pos.y + (global_.size.y - size.y) * 0.5f,
                                   t.text_size, titles_[i], t.text);
            }
            x += width + t.spacing;
        }

        append_focus_ring(list, {global_.pos.x, global_.pos.y, global_.size.x, global_.size.y}, t.radius_small);
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
            case event::KeyCode::Left: {
                // 子菜单里 Left 退回父层；顶层时切换到上一个菜单。
                if (active_depth_ > 0) {
                    close_deeper(active_depth_);
                    active_depth_--;
                    return true;
                }
                current_ = (open_ - 1 + menu_count()) % menu_count();
                open(current_);
                return true;
            }
            case event::KeyCode::Right: {
                auto* active = active_popup();
                const int highlight = active ? active->keyboard_highlight() : -1;
                if (active && highlight >= 0 &&
                    highlight < static_cast<int>(active->items().size()) &&
                    active->items()[static_cast<std::size_t>(highlight)].has_submenu()) {
                    open_submenu_from(active, highlight, active_depth_);
                    active_depth_ = std::min(active_depth_ + 1, static_cast<int>(popups_.size()) - 1);
                    if (auto* sub = active_popup()) {
                        sub->set_keyboard_highlight(first_actionable_of(sub->items()));
                    }
                    return true;
                }
                if (active_depth_ == 0) {
                    current_ = (open_ + 1) % menu_count();
                    open(current_);
                    return true;
                }
                return false;
            }
            case event::KeyCode::Down:
                move_highlight(1);
                return true;
            case event::KeyCode::Up:
                move_highlight(-1);
                return true;
            case event::KeyCode::Home:
                set_highlight_edge(false);
                return true;
            case event::KeyCode::End:
                set_highlight_edge(true);
                return true;
            case event::KeyCode::Enter:
            case event::KeyCode::Space: {
                auto* active = active_popup();
                if (!active) return false;
                activate(active, active->keyboard_highlight());
                return true;
            }
            default:
                return false;
        }
    }

    void on_focus_lost() override
    {
        Widget::on_focus_lost();
        hovered_ = -1;
        // 失焦时收起菜单：键盘 Tab 离开菜单栏不应该留下悬空的弹层。
        if (is_open()) close();
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
        // 上下都要留 padding，否则第一行上方有空隙、最后一行贴底。
        float height = t.padding * 2.0f;
        for (const auto& item : menus_[static_cast<std::size_t>(index)]) {
            height += item.separator ? 9.0f : t.control_height;
        }
        return height;
    }

    bool ensure_overlay()
    {
        auto* root = find_root();
        if (!root) return false;
        if (popup_ && scrim_) return true;

        close_all_popups();

        if (!scrim_) {
            auto scrim = std::make_unique<detail::MenuScrim>(
                [this]() { close(); },
                [this](glm::vec2 pos) { hover_titles(pos); });
            scrim_ = scrim.get();
            root->add_overlay(std::move(scrim));
        }

        auto popup = std::make_unique<detail::MenuPopup>(*this, 160.0f, 0.0f);
        auto* raw = popup.get();
        root->add_overlay(std::move(popup));

        popups_ = {raw};
        sub_items_ = {-1};
        popup_ = raw;
        return true;
    }

    void update_popup_geometry()
    {
        if (popups_.empty() || open_ < 0) return;

        const float x = title_left(open_);
        const float y = global_.pos.y + global_.size.y + 2.0f;
        popups_[0]->arrange({{x, y}, popups_[0]->local_bounds().size});

        auto* root = find_root();
        const float viewport_w = root ? root->global_bounds().size.x : 1.0e9f;

        for (std::size_t depth = 1; depth < popups_.size(); ++depth) {
            auto* parent = popups_[depth - 1];
            auto* sub = popups_[depth];
            const int item = sub_items_[depth];
            if (item < 0 || item >= static_cast<int>(parent->items().size())) break;

            const auto parent_bounds = parent->global_bounds();
            const glm::vec2 sub_size = sub->local_bounds().size;
            float sub_x = parent_bounds.pos.x + parent_bounds.size.x - 4.0f;
            // 右边界不够时翻到父菜单左侧。
            if (sub_x + sub_size.x > viewport_w) {
                sub_x = parent_bounds.pos.x - sub_size.x + 4.0f;
            }
            sub->arrange({{sub_x, parent->item_top(item)}, sub_size});
        }
    }

    static int first_actionable_of(const std::vector<MenuItem>& list)
    {
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (!list[i].separator && list[i].enabled) return static_cast<int>(i);
        }
        return -1;
    }

    static int last_actionable_of(const std::vector<MenuItem>& list)
    {
        for (int i = static_cast<int>(list.size()) - 1; i >= 0; --i) {
            const auto& item = list[static_cast<std::size_t>(i)];
            if (!item.separator && item.enabled) return i;
        }
        return -1;
    }

    int popup_depth(detail::MenuPopup* popup) const
    {
        for (std::size_t i = 0; i < popups_.size(); ++i) {
            if (popups_[i] == popup) return static_cast<int>(i);
        }
        return -1;
    }

    detail::MenuPopup* active_popup() const
    {
        if (popups_.empty()) return nullptr;
        const int depth = std::clamp(active_depth_, 0, static_cast<int>(popups_.size()) - 1);
        return popups_[static_cast<std::size_t>(depth)];
    }

    void close_deeper(int depth)
    {
        if (depth <= 0) return;
        auto* root = find_root();
        while (static_cast<int>(popups_.size()) > depth) {
            auto* popup = popups_.back();
            if (root) root->remove_overlay(popup);
            popups_.pop_back();
            sub_items_.pop_back();
        }
        if (active_depth_ >= static_cast<int>(popups_.size())) {
            active_depth_ = static_cast<int>(popups_.size()) - 1;
        }
    }

    void close_all_popups()
    {
        auto* root = find_root();
        for (auto* popup : popups_) {
            if (root) root->remove_overlay(popup);
        }
        popups_.clear();
        sub_items_.clear();
        popup_ = nullptr;
        active_depth_ = 0;
    }

    // 打开（或切换到）某一层菜单项的子菜单。
    void open_submenu_from(detail::MenuPopup* parent, int item_index, int depth)
    {
        if (!parent) return;
        if (depth < 0) depth = popup_depth(parent);
        if (depth < 0) return;

        const auto& list = parent->items();
        if (item_index < 0 || item_index >= static_cast<int>(list.size())) return;
        const auto& item = list[static_cast<std::size_t>(item_index)];
        if (!item.has_submenu()) {
            close_deeper(depth + 1);
            return;
        }

        // 已经是同一项：只更新几何。
        const std::size_t next = static_cast<std::size_t>(depth + 1);
        if (next < popups_.size() && sub_items_[next] == item_index) {
            update_popup_geometry();
            return;
        }

        close_deeper(depth + 1);
        auto* root = find_root();
        if (!root) return;

        auto sub = std::make_unique<detail::MenuPopup>(*this, 160.0f, 0.0f);
        auto* raw = sub.get();
        raw->set_items(item.children);
        root->add_overlay(std::move(sub));
        popups_.push_back(raw);
        sub_items_.push_back(item_index);
        update_popup_geometry();
    }

    // 鼠标悬停到某一层时调用：有子菜单就展开，否则收掉更深层。
    void on_popup_hover(detail::MenuPopup* popup, int index)
    {
        const int depth = popup_depth(popup);
        if (depth < 0) return;
        active_depth_ = depth;

        const auto& list = popup->items();
        if (index >= 0 && index < static_cast<int>(list.size()) &&
            list[static_cast<std::size_t>(index)].has_submenu()) {
            open_submenu_from(popup, index, depth);
        } else {
            close_deeper(depth + 1);
        }
    }

    void move_highlight(int delta)
    {
        auto* active = active_popup();
        if (!active) return;
        const auto& list = active->items();
        if (list.empty()) return;

        const int count = static_cast<int>(list.size());
        int index = active->keyboard_highlight();
        for (int step = 0; step < count; ++step) {
            index += delta;
            if (index < 0) index = count - 1;
            if (index >= count) index = 0;
            const auto& item = list[static_cast<std::size_t>(index)];
            if (!item.separator && item.enabled) {
                active->set_keyboard_highlight(index);
                return;
            }
        }
    }

    void set_highlight_edge(bool last)
    {
        auto* active = active_popup();
        if (!active) return;
        active->set_keyboard_highlight(last ? last_actionable_of(active->items())
                                            : first_actionable_of(active->items()));
    }

    void activate(detail::MenuPopup* popup, int index)
    {
        if (!popup || index < 0) return;
        const auto& list = popup->items();
        if (index >= static_cast<int>(list.size())) return;
        const auto& item = list[static_cast<std::size_t>(index)];
        if (item.separator || !item.enabled) return;

        if (item.has_submenu()) {
            open_submenu_from(popup, index, popup_depth(popup));
            return;
        }

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
    int active_depth_ = 0;
    detail::MenuPopup* popup_ = nullptr; // == popups_[0]
    detail::MenuScrim* scrim_ = nullptr;
    // 层级栈：popups_[0] 是顶层菜单，之后是逐层打开的子菜单。
    // 所有权在 root 的 overlay 列表，这里只记裸指针。
    std::vector<detail::MenuPopup*> popups_;
    // popups_[d] 的来源项索引（d >= 1）；与 popups_ 对齐。
    std::vector<int> sub_items_;
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

inline float MenuPopup::content_width() const
{
    const auto& t = owner_->ui_theme();
    float width = 160.0f;
    for (const auto& item : items_) {
        if (item.separator) continue;
        const float shortcut_w = item.shortcut.empty()
            ? 0.0f : owner_->text_width(item.shortcut) + t.padding * 2.0f;
        const float arrow_w = item.has_submenu() ? t.control_height * 0.5f : 0.0f;
        width = std::max(width, owner_->text_width(item.label) + shortcut_w + arrow_w + t.padding * 3.0f);
    }
    return width;
}

inline float MenuPopup::content_height() const
{
    const auto& t = owner_->ui_theme();
    float height = t.padding * 2.0f;
    for (const auto& item : items_) {
        height += item.separator ? separator_height() : row_height();
    }
    return height;
}

inline float MenuPopup::item_top(int index) const
{
    float y = global_.pos.y + owner_->ui_theme().padding;
    const int count = std::min(index, static_cast<int>(items_.size()));
    for (int i = 0; i < count; ++i) {
        y += items_[static_cast<std::size_t>(i)].separator ? separator_height() : row_height();
    }
    return y;
}

inline int MenuPopup::index_at(glm::vec2 pos) const
{
    if (!global_.contains(pos)) return -1;
    float y = global_.pos.y + owner_->ui_theme().padding;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const float row = items_[i].separator ? separator_height() : row_height();
        if (pos.y >= y && pos.y <= y + row) return items_[i].separator ? -1 : static_cast<int>(i);
        y += row;
    }
    return -1;
}

inline void MenuPopup::append_to(graphics::DrawList& list) const
{
    const auto& t = owner_->ui_theme();
    const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
    const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
    const float radius = corner_radius(t.radius_medium);

    graphics::append_rounded_rect(list, bounds, radius, surface.fill);
    if (surface.border_width > 0.0f) {
        graphics::append_rounded_ring(list, bounds, radius, surface.border_width, surface.border);
    }

    auto* atlas = owner_->font_atlas();
    float y = global_.pos.y + t.padding;
    const float line_h = atlas ? atlas->line_height(t.text_size) : t.text_size;
    const int highlighted_index = highlighted();

    for (std::size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        const float row = item.separator ? separator_height() : row_height();

        if (item.separator) {
            graphics::Rectangle line{global_.pos.x + t.padding, y + row * 0.5f,
                                     global_.size.x - t.padding * 2.0f, 1.0f, t.panel_border};
            line.append_to(list);
            y += row;
            continue;
        }

        const bool active = static_cast<int>(i) == highlighted_index && item.enabled;
        if (active) {
            const BoxStyle style = t.style(StyleRole::MenuItem, ControlState::Hovered);
            graphics::append_rounded_rect(list, {global_.pos.x + 2.0f, y, global_.size.x - 4.0f, row},
                                          corner_radius(style.radius), style.fill);
        }

        const glm::vec4 color = item.enabled ? t.text : t.text_disabled;
        const float arrow_space = item.has_submenu() ? t.control_height * 0.5f : 0.0f;
        if (atlas) {
            const float text_y = y + (row - line_h) * 0.5f;
            atlas->append_text(list, global_.pos.x + t.padding * 2.0f, text_y,
                               t.text_size, item.label, color);
            if (!item.shortcut.empty()) {
                const float shortcut_w = atlas->measure(item.shortcut, t.text_size).x;
                atlas->append_text(list,
                                   global_.pos.x + global_.size.x - t.padding * 2.0f - arrow_space - shortcut_w,
                                   text_y, t.text_size, item.shortcut, t.text_disabled);
            }
        }

        // 子菜单标记：右侧向三角。
        if (item.has_submenu()) {
            const float ax = global_.pos.x + global_.size.x - t.padding - 4.0f;
            const float ay = y + row * 0.5f;
            graphics::Triangle arrow{ax, ay - 4.0f, ax, ay + 4.0f, ax + 4.0f, ay,
                                     item.enabled ? t.text_muted : t.text_disabled};
            arrow.append_to(list);
        }
        y += row;
    }
}

inline bool MenuPopup::on_mouse_move(glm::vec2 pos)
{
    const int index = index_at(pos);
    if (index != hovered_) {
        hovered_ = index;
        keyboard_highlight_ = -1;
        owner_->on_popup_hover(this, index);
    }
    return true;
}

inline bool MenuPopup::on_click(glm::vec2 pos, int)
{
    const int index = index_at(pos);
    if (index < 0) return true;
    owner_->activate(this, index);
    return true;
}
} // namespace detail
} // namespace xel::ui
