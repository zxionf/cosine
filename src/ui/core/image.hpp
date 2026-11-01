#pragma once

#include "ui/theme.hpp"
#include "ui/widget.hpp"
#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "graphics/sprite.hpp"

#include <algorithm>

#include <cstdint>
#include <string>
#include <utility>

namespace xel::ui
{
// 图像控件：texture_id 非 0 时按 fit 绘制真实纹理（直角、可选底板/边框），
// 为 0 时画占位（圆角底色 + 边框 + 对角交叉 + 居中文案）。
class Image : public Widget
{
public:
    enum class Fit
    {
        Fill,    // 拉伸填满
        Contain, // 保持比例、完整显示
        Cover,   // 保持比例、填满并裁剪
    };

    Image(float width, float height)
    {
        set_pos(0.0f, 0.0f);
        set_size(width, height);
    }

    explicit Image(glm::vec2 size) : Image(size.x, size.y) {}

    void set_source(std::string path)
    {
        source_ = std::move(path);
        mark_layout_dirty();
    }
    const std::string& source() const { return source_; }

    // 纹理句柄：图片槽索引 + 1（与 scene::Node::texture_id、TextureCache 一致）；0 = 无纹理。
    void set_texture_id(std::uint32_t id) { texture_id_ = id; }
    std::uint32_t texture_id() const { return texture_id_; }

    // 纹理像素尺寸：Contain / Cover 需要宽高比；{0,0} 表示未知，按 Fill 处理。
    void set_texture_size(glm::vec2 size)
    {
        texture_size_ = size;
        mark_layout_dirty();
    }
    glm::vec2 texture_size() const { return texture_size_; }

    // 透明 PNG 的可选底板：默认不画，调用 set_background 时自动开启。
    void set_background(glm::vec4 color)
    {
        background_ = color;
        background_visible_ = true;
        mark_layout_dirty();
    }
    glm::vec4 background() const { return background_; }
    void set_background_visible(bool on)
    {
        background_visible_ = on;
        mark_layout_dirty();
    }
    bool background_visible() const { return background_visible_; }

    // 纹理模式下是否绘制外框（默认画，与占位一致）。
    void set_border_visible(bool on) { border_visible_ = on; }
    bool border_visible() const { return border_visible_; }

    void set_fit(Fit fit) { fit_ = fit; }
    Fit fit() const { return fit_; }

    // Fit 计算：只处理目标矩形与 UV，不依赖渲染层，便于单测。
    struct Placement
    {
        Rect dst;
        glm::vec4 uv{0.0f, 0.0f, 1.0f, 1.0f};
    };

    static Placement compute_placement(Rect rect, glm::vec2 texture_size, Fit fit)
    {
        if (texture_size.x <= 0.0f || texture_size.y <= 0.0f || fit == Fit::Fill) {
            return {rect, {0.0f, 0.0f, 1.0f, 1.0f}};
        }
        const float sx = rect.size.x / texture_size.x;
        const float sy = rect.size.y / texture_size.y;
        const float scale = fit == Fit::Contain ? std::min(sx, sy) : std::max(sx, sy);
        const glm::vec2 size{texture_size.x * scale, texture_size.y * scale};
        const glm::vec2 pos{rect.pos.x + (rect.size.x - size.x) * 0.5f,
                            rect.pos.y + (rect.size.y - size.y) * 0.5f};
        return {{pos, size}, {0.0f, 0.0f, 1.0f, 1.0f}};
    }

    void set_tint(glm::vec4 color)
    {
        tint_ = color;
        mark_layout_dirty();
    }
    glm::vec4 tint() const { return tint_; }

    void set_placeholder_text(std::string text)
    {
        placeholder_ = std::move(text);
        mark_layout_dirty();
    }
    // 未显式设置时取来源文件名；仍为空则回退 "image"。
    std::string placeholder_text() const
    {
        if (!placeholder_.empty()) return placeholder_;
        if (!source_.empty()) {
            const std::size_t slash = source_.find_last_of("/\\");
            return slash == std::string::npos ? source_ : source_.substr(slash + 1);
        }
        return "image";
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return clamp_size(local_bounds().size);
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const Rect rect{global_.pos, global_.size};
        if (texture_id_ != 0) {
            append_texture(list, rect);
            return;
        }
        append_placeholder(list, rect);
    }

private:
    void append_border(graphics::DrawList& list, Rect rect) const
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{rect.pos.x, rect.pos.y, rect.size.x, rect.size.y};
        const float w = t.border;
        const glm::vec4 border = t.panel_border;
        graphics::Line top{bounds.x, bounds.y, bounds.x + bounds.z, bounds.y, w, border};
        graphics::Line bottom{bounds.x, bounds.y + bounds.w, bounds.x + bounds.z,
                              bounds.y + bounds.w, w, border};
        graphics::Line left{bounds.x, bounds.y, bounds.x, bounds.y + bounds.w, w, border};
        graphics::Line right{bounds.x + bounds.z, bounds.y, bounds.x + bounds.z,
                             bounds.y + bounds.w, w, border};
        top.append_to(list);
        bottom.append_to(list);
        left.append_to(list);
        right.append_to(list);
    }

    void append_placeholder(graphics::DrawList& list, Rect rect) const
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{rect.pos.x, rect.pos.y, rect.size.x, rect.size.y};
        const float radius = corner_radius(t.radius);

        glm::vec4 bg = tint_ * t.accent;
        bg.a = 0.10f;
        graphics::append_rounded_rect(list, bounds, radius, bg);
        append_border(list, rect);

        const float w = t.border;
        glm::vec4 cross = t.panel_border;
        cross.a *= 0.6f;
        graphics::Line diag_a{bounds.x, bounds.y, bounds.x + bounds.z, bounds.y + bounds.w, w, cross};
        graphics::Line diag_b{bounds.x + bounds.z, bounds.y, bounds.x, bounds.y + bounds.w, w, cross};
        diag_a.append_to(list);
        diag_b.append_to(list);

        auto* atlas = font_atlas();
        if (atlas == nullptr) return;

        const std::string label = placeholder_text();
        const float size = t.small_text_size;
        const glm::vec2 text = atlas->measure(label, size);
        if (text.x > bounds.z - 4.0f) return; // 放不下就不画，避免溢出

        list.push_clip(bounds);
        atlas->append_text(list,
                           bounds.x + (bounds.z - text.x) * 0.5f,
                           bounds.y + (bounds.w - text.y) * 0.5f,
                           size,
                           label,
                           t.text_muted,
                           {bounds.x, bounds.y, bounds.x + bounds.z, bounds.y + bounds.w});
        list.pop_clip();
    }

    void append_texture(graphics::DrawList& list, Rect rect) const
    {
        const glm::vec4 bounds{rect.pos.x, rect.pos.y, rect.size.x, rect.size.y};
        if (background_visible_) {
            graphics::append_rounded_rect(list, bounds, 0.0f, background_);
        }

        const std::uint32_t slot = graphics::image_slot(texture_id_ - 1);
        const Placement placement = compute_placement(rect, texture_size_, fit_);
        if (placement.dst.size.x <= 0.0f || placement.dst.size.y <= 0.0f) return;

        // Cover 会超出 rect，统一用裁剪保证不溢出；Contain 的留白不绘制。
        list.push_clip(bounds);
        graphics::Sprite{placement.dst.pos.x, placement.dst.pos.y, placement.dst.size.x,
                         placement.dst.size.y, slot, tint_, placement.uv}
            .append_to(list);
        list.pop_clip();

        if (border_visible_) append_border(list, rect);
    }

private:
    std::string source_;
    std::string placeholder_;
    std::uint32_t texture_id_ = 0;
    glm::vec2 texture_size_{0.0f, 0.0f};
    glm::vec4 background_{0.0f, 0.0f, 0.0f, 1.0f};
    bool background_visible_ = false;
    bool border_visible_ = true;
    Fit fit_ = Fit::Contain;
    glm::vec4 tint_{1.0f, 1.0f, 1.0f, 1.0f};
};
} // namespace xel::ui
