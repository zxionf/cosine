#pragma once

#include "theme.hpp"
#include "style.hpp"
#include "widget.hpp"
#include "root.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/box.hpp"
#include "../graphics/rectangle.hpp"
#include "../graphics/triangle.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace xel::ui
{
class Dropdown;

namespace detail
{
// 全屏透明遮罩：点击任意位置关闭下拉，避免事件穿透到下层控件。
class DropdownScrim : public Widget
{
public:
    explicit DropdownScrim(std::function<void()> dismiss) : dismiss_{std::move(dismiss)} {}
    glm::vec2 measure(glm::vec2 size) override { return size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList&) const override {}
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

private:
    std::function<void()> dismiss_;
};

// 下拉列表浮层：绘制选项、处理悬停与点击。
class DropdownList : public Widget
{
public:
    DropdownList(Dropdown& owner, float width, float height) : owner_{&owner}
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
    float row_height() const;

    int hovered() const { return hovered_; }

private:
    int index_at(glm::vec2 pos) const;
    Dropdown* owner_ = nullptr;
    int hovered_ = -1;
};
} // namespace detail

// 下拉选择：点击展开 overlay 列表，支持方向键、Home/End、Enter 选择与 Esc 关闭。
class Dropdown : public Widget
{
public:
    explicit Dropdown(std::vector<std::string> options = {}, int selected = 0)
        : options_{std::move(options)}
    {
        if (!options_.empty()) {
            selected_ = std::clamp(selected, 0, static_cast<int>(options_.size()) - 1);
        }
    }


    void set_options(std::vector<std::string> options)
    {
        options_ = std::move(options);
        selected_ = options_.empty() ? -1 : std::clamp(selected_, 0, static_cast<int>(options_.size()) - 1);
        if (!options_.empty() && selected_ < 0) selected_ = 0;
        mark_layout_dirty();
    }

    const std::vector<std::string>& options() const { return options_; }
    int selected() const { return selected_; }
    const std::string& selected_text() const
    {
        static const std::string empty;
        return (selected_ >= 0 && selected_ < static_cast<int>(options_.size()))
            ? options_[static_cast<std::size_t>(selected_)]
            : empty;
    }

    void set_selected(int index, bool notify = true)
    {
        if (options_.empty()) return;
        const int clamped = std::clamp(index, 0, static_cast<int>(options_.size()) - 1);
        if (clamped == selected_) return;
        selected_ = clamped;
        if (notify && on_change_) on_change_(selected_);
    }

    void set_on_change(std::function<void(int)> handler) { on_change_ = std::move(handler); }

    bool is_open() const { return popup_ != nullptr; }
    int highlight() const { return highlight_; }

    void open()
    {
        if (is_open() || options_.empty()) return;

        auto* root = find_root();
        if (!root) return;

        const auto& t = ui_theme();
        const float row = row_height();
        const float width = std::max(global_.size.x, min_width());
        // 上下各留一份 padding：否则行从顶部开始画，底部会多出空白、顶部贴边。
        const float height = static_cast<float>(options_.size()) * row + t.padding * 2.0f;

        auto scrim = std::make_unique<detail::DropdownScrim>([this]() { close(); });
        scrim->set_pos(0.0f, 0.0f);
        scrim_ = scrim.get();

        auto popup = std::make_unique<detail::DropdownList>(*this, width, height);
        popup->set_pos(global_.pos.x, global_.pos.y + global_.size.y + 2.0f);
        popup_ = popup.get();

        root->add_overlay(std::move(scrim));
        root->add_overlay(std::move(popup));
        highlight_ = selected_;
    }

    void close()
    {
        if (!popup_ && !scrim_) return;
        if (auto* root = find_root()) {
            root->remove_overlay(popup_);
            root->remove_overlay(scrim_);
        }
        popup_ = nullptr;
        scrim_ = nullptr;
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        const auto& t = ui_theme();
        return {min_width(), t.control_height};
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = visual_style(StyleRole::Control,
                                            control_state(enabled_, is_hovered() || is_open(), pressed_));
        const float radius = corner_radius(style.radius);

        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        // 编辑/展开中始终强调边框；焦点环只在 focus_policy 允许时叠加。
        const glm::vec4 border_color = is_focused() ? t.accent : t.panel_border;
        if (t.border > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, t.border, border_color);
        }
        append_focus_ring(list, bounds, radius);

        if (auto* atlas = font_atlas()) {
            const glm::vec2 text_size = atlas->measure(selected_text(), style.text_size);
            atlas->append_text(list,
                               global_.pos.x + t.padding,
                               global_.pos.y + (global_.size.y - text_size.y) * 0.5f,
                               style.text_size, selected_text(), style.text);
        }

        // 下拉箭头
        const float ax = global_.pos.x + global_.size.x - t.padding - 5.0f;
        const float ay = global_.pos.y + global_.size.y * 0.5f;
        graphics::Triangle arrow{ax, ay - 2.0f, ax + 10.0f, ay - 2.0f, ax + 5.0f, ay + 3.0f, style.text};
        arrow.append_to(list);
    }

    bool focusable() const override { return true; }
    bool hit_testable() const override { return true; }

    void on_mouse_leave() override { pressed_ = false; }
    bool on_mouse_down(glm::vec2, int button) override
    {
        if (!enabled_ || button != 0) return false;
        pressed_ = true;
        if (is_open()) close();
        else open();
        return true;
    }
    bool on_mouse_up(glm::vec2, int) override
    {
        pressed_ = false;
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!enabled_) return false;
        if (!is_open()) {
            if (key == event::KeyCode::Space || key == event::KeyCode::Enter
                || key == event::KeyCode::KeypadEnter || key == event::KeyCode::Down) {
                open();
                return true;
            }
            return false;
        }

        switch (key) {
            case event::KeyCode::Up:
                highlight_ = std::max(0, highlight_ - 1);
                return true;
            case event::KeyCode::Down:
                highlight_ = std::min(static_cast<int>(options_.size()) - 1, highlight_ + 1);
                return true;
            case event::KeyCode::Home:
                highlight_ = 0;
                return true;
            case event::KeyCode::End:
                highlight_ = static_cast<int>(options_.size()) - 1;
                return true;
            case event::KeyCode::Enter:
            case event::KeyCode::KeypadEnter:
            case event::KeyCode::Space:
                commit(highlight_);
                return true;
            case event::KeyCode::Tab:
                close();
                return false;
            case event::KeyCode::Escape:
                close();
                return true;
            default:
                return false;
        }
    }

    void on_focus_lost() override
    {
        // 必须先走基类：否则 focused_ 保持 true，边框会一直用强调色。
        Widget::on_focus_lost();
        close();
    }

    // 供列表浮层回调。
    void commit(int index)
    {
        set_selected(index);
        close();
    }

    float row_height() const { return row_height_static(*this); }

    float list_width() const { return std::max(global_.size.x, min_width()); }

private:
    static float row_height_static(const Dropdown& d)
    {
        return d.ui_theme().control_height;
    }

    float min_width() const
    {
        const auto& t = ui_theme();
        float text_w = 0.0f;
        if (auto* atlas = font_atlas()) {
            for (const auto& option : options_) {
                text_w = std::max(text_w, atlas->measure(option, t.text_size).x);
            }
        } else {
            for (const auto& option : options_) {
                text_w = std::max(text_w, static_cast<float>(option.size()) * t.text_size * 0.5f);
            }
        }
        // 列表至少 160px，避免选项较短时弹层过窄、与下层文本重叠。
        return std::max(160.0f, text_w + t.padding * 4.0f + 24.0f);
    }

    UIRoot* find_root() const
    {
        for (Widget* widget = const_cast<Dropdown*>(this); widget != nullptr; widget = widget->parent()) {
            if (auto* root = dynamic_cast<UIRoot*>(widget)) return root;
        }
        return nullptr;
    }

    std::vector<std::string> options_;
    int selected_ = -1;
    int highlight_ = 0;
    bool enabled_ = true;
    bool pressed_ = false;
    detail::DropdownList* popup_ = nullptr;
    detail::DropdownScrim* scrim_ = nullptr;
    std::function<void(int)> on_change_;
};

namespace detail
{
inline float DropdownList::row_height() const
{
    return owner_ ? owner_->row_height() : ui_theme().control_height;
}

inline int DropdownList::index_at(glm::vec2 pos) const
{
    if (!owner_) return -1;
    const float row = row_height();
    const float top = global_.pos.y + ui_theme().padding;
    if (pos.y < top) return -1;
    const int index = static_cast<int>((pos.y - top) / row);
    if (index < 0 || index >= static_cast<int>(owner_->options().size())) return -1;
    return index;
}

inline void DropdownList::append_to(graphics::DrawList& list) const
{
    if (!owner_) return;
    const auto& t = ui_theme();
    const float row = row_height();
    const float width = global_.size.x;
    const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
    const float radius = corner_radius(t.radius_medium);
    const glm::vec4 bounds{global_.pos.x, global_.pos.y, width, global_.size.y};

    graphics::append_rounded_rect(list, bounds, radius, surface.fill);
    if (surface.border_width > 0.0f) {
        graphics::append_rounded_ring(list, bounds, radius, surface.border_width, surface.border);
    }

    const auto& options = owner_->options();
    const float top = global_.pos.y + t.padding;
    for (std::size_t i = 0; i < options.size(); ++i) {
        const float y = top + static_cast<float>(i) * row;
        const bool highlighted = static_cast<int>(i) == hovered_
            || (hovered_ < 0 && static_cast<int>(i) == owner_->highlight());
        const bool current = static_cast<int>(i) == owner_->selected();

        if (highlighted || current) {
            const BoxStyle item = t.style(StyleRole::MenuItem,
                                          highlighted ? ControlState::Hovered : ControlState::Pressed);
            graphics::append_rounded_rect(list, {global_.pos.x + 2.0f, y, width - 4.0f, row},
                                          corner_radius(item.radius), item.fill);
        }

        if (auto* atlas = owner_->font_atlas()) {
            const glm::vec2 text_size = atlas->measure(options[i], t.text_size);
            atlas->append_text(list, global_.pos.x + t.padding,
                               y + (row - text_size.y) * 0.5f,
                               t.text_size, options[i], t.text);
        }
    }
}

inline bool DropdownList::on_mouse_move(glm::vec2 pos)
{
    const int index = index_at(pos);
    if (index != hovered_) {
        hovered_ = index;
        mark_layout_dirty();
    }
    return index >= 0;
}

inline bool DropdownList::on_click(glm::vec2 pos, int)
{
    if (!owner_) return false;
    const int index = index_at(pos);
    if (index < 0) return false;
    owner_->commit(index);
    return true;
}
} // namespace detail
} // namespace xel::ui
