#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/leaf_node.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace xel::ui
{
class Tooltip : public LeafNode
{
public:
    explicit Tooltip(std::string text = {})
        : text_{std::move(text)}
    {
        set_visible(false);
    }

    void set_text(std::string text) { text_ = std::move(text); mark_layout_dirty(); }
    const std::string& text() const { return text_; }

    // 悬停多久后显示；0 表示立即显示。
    void set_delay(float seconds) { delay_ = std::max(0.0f, seconds); }
    float delay() const { return delay_; }

    void show_at(glm::vec2 pos)
    {
        set_pos(pos);
        set_visible(true);
    }

    void hide() { set_visible(false); }

    // 每帧驱动：over_target 表示鼠标是否停在提示目标上。
    // 悬停累计到 delay_ 后显示；显示后跟随 cursor，并在 bounds 内做翻转与夹取。
    void update_hover(float dt, bool over_target, glm::vec2 cursor, Rect bounds)
    {
        if (!over_target) {
            hover_time_ = 0.0f;
            if (visible()) hide();
            return;
        }

        hover_time_ += std::max(0.0f, dt);
        if (!visible() && hover_time_ < delay_) return;

        const glm::vec2 size = tooltip_size();
        glm::vec2 pos = cursor + glm::vec2{kOffsetX, kOffsetY};

        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            // 右下放不下就翻到左上，再夹回可用区域。
            if (pos.x + size.x > bounds.pos.x + bounds.size.x) {
                pos.x = cursor.x - kOffsetX - size.x;
            }
            if (pos.y + size.y > bounds.pos.y + bounds.size.y) {
                pos.y = cursor.y - kOffsetY - size.y;
            }
            pos.x = std::clamp(pos.x, bounds.pos.x,
                               bounds.pos.x + std::max(0.0f, bounds.size.x - size.x));
            pos.y = std::clamp(pos.y, bounds.pos.y,
                               bounds.pos.y + std::max(0.0f, bounds.size.y - size.y));
            set_pos(pos - bounds.pos);
        } else {
            set_pos(pos);
        }

        if (!visible()) set_visible(true);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return tooltip_size();
    }


    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = t.style(StyleRole::Tooltip, ControlState::Normal);
        const float radius = corner_radius(style.radius);

        // tooltip_bg 是半透明色：圆角几何用九宫格不重叠切分，不会叠出亮边。
        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, style.border);
        }

        if (auto* atlas = font_atlas()) {
            // UI-41：字号来自 Tooltip 角色的 Small 档，不再直读 small_text_size。
            atlas->append_text(list, global_.pos.x + t.padding, global_.pos.y + t.padding * 0.5f,
                               style.text_size, text_, style.text);
        }
    }

    bool hit_testable() const override { return false; }
    // Tooltip 是浮层，绘制在普通内容之上。
    Layer layer() const override { return Layer::Floating; }

private:
    static constexpr float kOffsetX = 16.0f;
    static constexpr float kOffsetY = 18.0f;

    glm::vec2 tooltip_size() const
    {
        const auto& t = ui_theme();
        const float size = t.style(StyleRole::Tooltip, ControlState::Normal).text_size;
        glm::vec2 text_size{graphics::FontAtlas::estimate_text_width(text_, size), size};
        if (auto* atlas = font_atlas()) {
            text_size = atlas->measure(text_, size);
        }
        return {text_size.x + t.padding * 2.0f, text_size.y + t.padding};
    }

    std::string text_;
    float delay_ = 0.35f;
    float hover_time_ = 0.0f;
};
} // namespace xel::ui
