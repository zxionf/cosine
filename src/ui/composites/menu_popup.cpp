// 菜单浮层的实现。MenuScrim / MenuPopup 是独立可测的浮层控件，
// 与 MenuBar 的状态机不是一类东西，所以不放在 menu_bar.hpp 里。
#include "ui/composites/menu_popup.hpp"

#include "ui/composites/menu_bar.hpp"

#include "graphics/box.hpp"
#include "graphics/rectangle.hpp"
#include "graphics/triangle.hpp"

#include <algorithm>

namespace xel::ui
{
namespace detail
{
// ── MenuScrim：全屏遮罩 ────────────────────────────────────────────
void MenuScrim::append_to(graphics::DrawList& list) const
{
    graphics::Rectangle backdrop{bounds_.pos.x, bounds_.pos.y, bounds_.size.x, bounds_.size.y,
                                 glm::vec4{0.0f, 0.0f, 0.0f, 0.05f}};
    backdrop.append_to(list);
}

bool MenuScrim::on_mouse_down(glm::vec2, int)
{
    if (dismiss_) dismiss_();
    return true;
}

bool MenuScrim::on_scroll(glm::vec2, glm::vec2)
{
    if (dismiss_) dismiss_();
    return true;
}

bool MenuScrim::on_mouse_move(glm::vec2 pos)
{
    if (hover_bar_) hover_bar_(pos);
    return true;
}

// ── 菜单内容访问（MenuBar 的 friend） ─────────────────────────────
const std::vector<MenuItem>& menu_items(const MenuBar& bar, int index)
{
    static const std::vector<MenuItem> empty;
    if (index < 0 || index >= bar.menu_count()) return empty;
    return bar.menus_[static_cast<std::size_t>(index)];
}

// ── MenuPopup ─────────────────────────────────────────────────────
MenuPopup::MenuPopup(MenuBar& owner, float width, float height)
    : owner_{&owner}
{
    // 尺寸由 arrange() 经 set_geometry 写入；不要设置 min/max_size，
    // 否则 clamp_size 会把后续尺寸锁死在创建时的值上。
    set_geometry({{0.0f, 0.0f}, {width, height}});
    set_preferred_size({width, height});
}

void MenuPopup::set_items(std::vector<MenuItem> items)
{
    items_ = std::move(items);
    hovered_ = -1;
    keyboard_highlight_ = -1;
    const glm::vec2 size = {content_width(), content_height()};
    // 自然尺寸随内容走（浮层不用 measure 协商，measure() 报这个值）：
    // 不更新 preferred 的话，root 重排会把浮层按构造期的 0 高摆放。
    set_preferred_size(size);
    set_geometry({bounds().pos, size});
}

float MenuPopup::row_height() const
{
    return owner_->ui_theme().control_height;
}

float MenuPopup::separator_height() const
{
    return 9.0f;
}

float MenuPopup::content_width() const
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

float MenuPopup::content_height() const
{
    const auto& t = owner_->ui_theme();
    float height = t.padding * 2.0f;
    for (const auto& item : items_) {
        height += item.separator ? separator_height() : row_height();
    }
    return height;
}

float MenuPopup::item_top(int index) const
{
    float y = bounds_.pos.y + owner_->ui_theme().padding;
    const int count = std::min(index, static_cast<int>(items_.size()));
    for (int i = 0; i < count; ++i) {
        y += items_[static_cast<std::size_t>(i)].separator ? separator_height() : row_height();
    }
    return y;
}

int MenuPopup::index_at(glm::vec2 pos) const
{
    if (!bounds_.contains(pos)) return -1;
    float y = bounds_.pos.y + owner_->ui_theme().padding;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const float row = items_[i].separator ? separator_height() : row_height();
        if (pos.y >= y && pos.y <= y + row) return items_[i].separator ? -1 : static_cast<int>(i);
        y += row;
    }
    return -1;
}

void MenuPopup::append_to(graphics::DrawList& list) const
{
    const auto& t = owner_->ui_theme();
    const glm::vec4 bounds{bounds_.pos.x, bounds_.pos.y, bounds_.size.x, bounds_.size.y};
    const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
    const float radius = corner_radius(t.radius_medium);

    graphics::append_rounded_rect(list, bounds, radius, surface.fill);
    if (surface.border_width > 0.0f) {
        graphics::append_rounded_ring(list, bounds, radius, surface.border_width, surface.border);
    }

    auto* atlas = owner_->font_atlas();
    float y = bounds_.pos.y + t.padding;
    const float line_h = atlas ? atlas->line_height(t.text_size) : t.text_size;
    const int highlighted_index = highlighted();

    for (std::size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        const float row = item.separator ? separator_height() : row_height();

        if (item.separator) {
            graphics::Rectangle line{bounds_.pos.x + t.padding, y + row * 0.5f,
                                     bounds_.size.x - t.padding * 2.0f, 1.0f, t.panel_border};
            line.append_to(list);
            y += row;
            continue;
        }

        const bool active = static_cast<int>(i) == highlighted_index && item.enabled;
        if (active) {
            const BoxStyle style = t.style(StyleRole::MenuItem, ControlState::Hovered);
            graphics::append_rounded_rect(list, {bounds_.pos.x + 2.0f, y, bounds_.size.x - 4.0f, row},
                                          corner_radius(style.radius), style.fill);
        }

        // 菜单项的文字色/字号统一从 MenuItem 角色取（Disabled 由角色自动切 text_disabled）。
        const BoxStyle item_style = t.style(StyleRole::MenuItem,
                                            item.enabled ? ControlState::Normal : ControlState::Disabled);
        const glm::vec4 color = item_style.text;
        const float arrow_space = item.has_submenu() ? t.control_height * 0.5f : 0.0f;
        if (atlas) {
            const float text_y = y + (row - line_h) * 0.5f;
            atlas->append_text(list, bounds_.pos.x + t.padding * 2.0f, text_y,
                               item_style.text_size, item.label, color);
            if (!item.shortcut.empty()) {
                // 快捷键属于辅助信息，用 Small 档。
                const float sc_size = t.text_size_of(TextSize::Small);
                const float shortcut_w = atlas->measure(item.shortcut, sc_size).x;
                const float sc_y = y + (row - atlas->line_height(sc_size)) * 0.5f;
                atlas->append_text(list,
                                   bounds_.pos.x + bounds_.size.x - t.padding * 2.0f - arrow_space - shortcut_w,
                                   sc_y, sc_size, item.shortcut, t.text_disabled);
            }
        }

        // 子菜单标记：右侧向三角。
        if (item.has_submenu()) {
            const float ax = bounds_.pos.x + bounds_.size.x - t.padding - 4.0f;
            const float ay = y + row * 0.5f;
            graphics::Triangle arrow{ax, ay - 4.0f, ax, ay + 4.0f, ax + 4.0f, ay,
                                     item.enabled ? t.text_muted : t.text_disabled};
            arrow.append_to(list);
        }
        y += row;
    }
}

bool MenuPopup::on_mouse_move(glm::vec2 pos)
{
    const int index = index_at(pos);
    if (index != hovered_) {
        hovered_ = index;
        keyboard_highlight_ = -1;
        owner_->on_popup_hover(this, index);
    }
    return true;
}

bool MenuPopup::on_click(glm::vec2 pos, int)
{
    const int index = index_at(pos);
    if (index < 0) return true;
    owner_->activate(this, index);
    return true;
}
} // namespace detail
} // namespace xel::ui
