#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "events/event.hpp"
#include "graphics/box.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <vector>

namespace xel::ui
{
// 颜色输入：一排预设色块 + 当前色十六进制显示。
// 点击色块即选中；键盘 Left/Right 在预设间移动。
// 十六进制文本只读（文本输入留待 TextInput 组合，见 M6.2 边界）。
class ColorInput : public Widget
{
public:
    explicit ColorInput(std::string label = {}, float width = 250.0f)
        : label_(std::move(label)), preferred_width_(width)
    {
        palette_ = default_palette();
    }

    void set_palette(std::vector<glm::vec4> palette)
    {
        if (!palette.empty()) palette_ = std::move(palette);
    }

    void set_value(glm::vec4 color)
    {
        color_ = glm::vec4{std::clamp(color.r, 0.0f, 1.0f), std::clamp(color.g, 0.0f, 1.0f),
                           std::clamp(color.b, 0.0f, 1.0f), std::clamp(color.a, 0.0f, 1.0f)};
    }

    glm::vec4 value() const { return color_; }

    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

    void set_on_change(std::function<void(glm::vec4)> handler) { handler_ = std::move(handler); }
    // 选中色块：上报 (before, after)，供编辑器生成一条撤销命令。
    void set_on_commit(std::function<void(glm::vec4, glm::vec4)> handler)
    {
        commit_ = std::move(handler);
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        // 面板变窄时跟随可用宽度。
        const float available = constraints.x;
        const float width = (available > 0.0f && available < 1.0e8f)
                                ? std::min(preferred_width_, available)
                                : preferred_width_;
        return {width, ui_theme().control_height};
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
        ControlState state = ControlState::Normal;
        if (!enabled_) {
            state = ControlState::Disabled;
        } else if (is_hovered()) {
            state = ControlState::Hovered;
        }

        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle style = visual_style(StyleRole::Input, state);
        const float radius = corner_radius(style.radius);
        graphics::append_rounded_rect(list, bounds, radius, style.fill);
        if (style.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, radius, style.border_width, style.border);
        }

        const float pad = std::max(4.0f, t.padding * 0.5f);

        auto* atlas = font_atlas();
        if (atlas != nullptr && !label_.empty()) {
            const float line = atlas->line_height(t.text_size);
            atlas->append_text(list, global_.pos.x + pad,
                               global_.pos.y + (global_.size.y - line) * 0.5f, t.text_size,
                               label_, enabled_ ? t.text : t.text_disabled);
        }

        // 色块矩形统一走 swatch_rect()：像素取整，避免 2px 环落在半像素上造成厚薄不均。
        for (std::size_t i = 0; i < palette_.size(); ++i) {
            const glm::vec4 rect = swatch_rect(i);
            const float swatch_radius = std::min(radius, rect.z * 0.5f);
            graphics::append_rounded_rect(list, rect, swatch_radius, palette_[i]);
            if (same_color(palette_[i], color_)) {
                graphics::append_rounded_ring(list, rect, swatch_radius, 2.0f,
                                              enabled_ ? t.accent : t.text_disabled);
            }
        }

        // 右侧十六进制：空间不足时不画，避免和色块挤在一起。
        const std::string text = hex();
        if (atlas != nullptr) {
            const float text_width = atlas->measure_width(text, t.text_size);
            const float right = global_.pos.x + global_.size.x - pad;
            const glm::vec4 last_swatch = swatch_rect(palette_.size() - 1);
            if (right - (last_swatch.x + last_swatch.z) >= text_width + pad) {
                const float line = atlas->line_height(t.text_size);
                atlas->append_text(list, right - text_width,
                                   global_.pos.y + (global_.size.y - line) * 0.5f, t.text_size,
                                   text, enabled_ ? t.text : t.text_disabled);
            }
        }
    }

    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!enabled_) return false;
        const int index = swatch_hit(pos);
        if (index < 0) return false;

        const glm::vec4 before = color_;
        set_value(palette_[static_cast<std::size_t>(index)]);
        notify();
        if (commit_ && !same_color(before, color_)) commit_(before, color_);
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (!enabled_ || palette_.empty()) return false;

        int current = -1;
        for (std::size_t i = 0; i < palette_.size(); ++i) {
            if (same_color(palette_[i], color_)) {
                current = static_cast<int>(i);
                break;
            }
        }
        if (current < 0) current = 0;

        const int count = static_cast<int>(palette_.size());
        if (key == event::KeyCode::Left) {
            const glm::vec4 before = color_;
            set_value(palette_[static_cast<std::size_t>((current + count - 1) % count)]);
            notify();
            if (commit_ && !same_color(before, color_)) commit_(before, color_);
            return true;
        }
        if (key == event::KeyCode::Right) {
            const glm::vec4 before = color_;
            set_value(palette_[static_cast<std::size_t>((current + 1) % count)]);
            notify();
            if (commit_ && !same_color(before, color_)) commit_(before, color_);
            return true;
        }
        return false;
    }

private:
    static std::vector<glm::vec4> default_palette()
    {
        return {
            {0.95f, 0.55f, 0.35f, 1.0f},
            {0.95f, 0.85f, 0.35f, 1.0f},
            {0.45f, 0.85f, 0.55f, 1.0f},
            {0.45f, 0.65f, 0.95f, 1.0f},
            {0.85f, 0.45f, 0.85f, 1.0f},
            {0.92f, 0.94f, 0.98f, 1.0f},
        };
    }

    static bool same_color(glm::vec4 a, glm::vec4 b)
    {
        constexpr float kEps = 0.01f;
        return std::fabs(a.r - b.r) < kEps && std::fabs(a.g - b.g) < kEps
            && std::fabs(a.b - b.b) < kEps;
    }

    std::string hex() const
    {
        const auto to_byte = [](float v) {
            return static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
        };
        return std::format("#{:02X}{:02X}{:02X}", to_byte(color_.r), to_byte(color_.g),
                           to_byte(color_.b));
    }

    // 色块矩形：绘制与命中共用。位置严格取整，否则 2px 选中环会落在半像素上，
    // 上/左被削薄、下/右显得厚。
    glm::vec4 swatch_rect(std::size_t index) const
    {
        const auto& t = ui_theme();
        const float pad = std::max(4.0f, t.padding * 0.5f);
        const float swatch = std::max(12.0f, std::round(global_.size.y - pad * 2.0f));

        float x = global_.pos.x + pad;
        if (!label_.empty()) {
            if (auto* atlas = font_atlas()) {
                x += atlas->measure_width(label_, t.text_size) + pad;
            }
        }
        x += (swatch + pad * 0.5f) * static_cast<float>(index);

        return {std::round(x), std::round(global_.pos.y + pad), swatch, swatch};
    }

    int swatch_hit(glm::vec2 pos) const
    {
        for (std::size_t i = 0; i < palette_.size(); ++i) {
            const glm::vec4 rect = swatch_rect(i);
            if (pos.x >= rect.x && pos.x <= rect.x + rect.z && pos.y >= rect.y
                && pos.y <= rect.y + rect.w) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void notify()
    {
        if (handler_) handler_(color_);
    }

    std::vector<glm::vec4> palette_;
    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};
    std::string label_;
    float preferred_width_ = 250.0f;
    bool enabled_ = true;
    std::function<void(glm::vec4)> handler_;
    std::function<void(glm::vec4, glm::vec4)> commit_;
};
} // namespace xel::ui
