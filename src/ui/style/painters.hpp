#pragma once

#include "graphics/box.hpp"
#include "graphics/drawable.hpp"
#include "graphics/font_atlas.hpp"
#include "ui/geom/rect.hpp"
#include "ui/style/style_types.hpp"
#include "ui/style/theme.hpp"

#include <string_view>

namespace xel::ui::paint
{
// 绘制部件（设计 §4.7）：把每个控件 append_to 里反复抄的三件事收成一处。
//
// 之前的状态（实测）：`append_focus_ring` 13 处手写、"fill + 可选边框" 12 处重复、
// "文字水平垂直居中" 6 处各写 4~6 行。它们都是**纯函数**（不持有状态、不做决策），
// 所以做成自由函数而不是部件对象：零存储、零虚函数、编译器可内联。

// 底 + 边框（边框宽度为 0 时自动跳过）。radius 传已算好的圆角档位。
inline void box(graphics::DrawList& list, Rect bounds, const BoxStyle& style, float radius)
{
    const glm::vec4 rect{bounds.pos.x, bounds.pos.y, bounds.size.x, bounds.size.y};
    graphics::append_rounded_rect(list, rect, radius, style.fill);
    if (style.border_width > 0.0f) {
        graphics::append_rounded_ring(list, rect, radius, style.border_width, style.border);
    }
}

// 焦点环。ring / border 两种风格由主题的 focus_style 决定（UI-39 后的唯一来源）。
// 传 visible = 控件的 focus_visible()：本函数不查焦点策略，只管画。
inline void focus_ring(graphics::DrawList& list, Rect bounds, float radius, const Theme& theme,
                       bool visible)
{
    if (!visible) return;
    const float thickness = theme.focus_thickness;
    const glm::vec4 rect{bounds.pos.x, bounds.pos.y, bounds.size.x, bounds.size.y};
    if (theme.focus_style == FocusStyle::Border) {
        graphics::append_rounded_rect(list, {rect.x, rect.y, rect.z, thickness}, 0.0f, theme.accent);
        graphics::append_rounded_rect(list, {rect.x, rect.y + rect.w - thickness, rect.z, thickness},
                                      0.0f, theme.accent);
        graphics::append_rounded_rect(list, {rect.x, rect.y, thickness, rect.w}, 0.0f, theme.accent);
        graphics::append_rounded_rect(list, {rect.x + rect.z - thickness, rect.y, thickness, rect.w},
                                      0.0f, theme.accent);
        return;
    }
    const float offset = theme.focus_offset;
    graphics::append_rounded_ring(
        list,
        {rect.x - offset, rect.y - offset, rect.z + offset * 2.0f, rect.w + offset * 2.0f},
        radius + offset, thickness, theme.accent);
}

// 「框 + 边框 + 焦点环」这一套（Button / Checkbox 框体 / Radio / Dropdown 框都用它）。
inline void box_with_focus(graphics::DrawList& list, Rect bounds, const BoxStyle& style, float radius,
                           const Theme& theme, bool focus_visible)
{
    box(list, bounds, style, radius);
    focus_ring(list, bounds, radius, theme, focus_visible);
}

// 文本尺寸（无图集时按 0.5 字宽估算，与 Label 的降级口径一致）。
inline glm::vec2 text_size(graphics::FontAtlas* atlas, std::string_view text, float size)
{
    if (atlas != nullptr) return atlas->measure(text, size);
    return {static_cast<float>(text.size()) * size * 0.5f, size};
}

// 单行文本在 bounds 内水平 + 垂直居中。返回实际绘制位置（调试/命中复用）。
inline glm::vec2 centered_text(graphics::DrawList& list, Rect bounds, std::string_view text,
                               const BoxStyle& style, graphics::FontAtlas* atlas)
{
    if (atlas == nullptr || text.empty()) return bounds.pos;
    const glm::vec2 size = atlas->measure(text, style.text_size);
    const glm::vec2 pos{bounds.pos.x + (bounds.size.x - size.x) * 0.5f,
                        bounds.pos.y + (bounds.size.y - size.y) * 0.5f};
    atlas->append_text(list, pos.x, pos.y, style.text_size, text, style.text);
    return pos;
}
} // namespace xel::ui::paint
