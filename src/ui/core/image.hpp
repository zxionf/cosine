#pragma once

#include "ui/widget.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace xel::ui
{
// 图像占位控件：source / texture_id / fit 接口已定，真实纹理采样等 M6 资源管线。
// 当前绘制为占位（圆角底色 + 边框 + 对角交叉 + 居中文案），
// 即使设置了 texture_id 也不做采样，避免「看起来支持纹理、实际是空图」的误判。
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

    // 预留纹理句柄：M6 资源管线就绪后由外部绑定；0 表示无纹理。
    void set_texture_id(std::uint32_t id) { texture_id_ = id; }
    std::uint32_t texture_id() const { return texture_id_; }

    void set_fit(Fit fit) { fit_ = fit; }
    Fit fit() const { return fit_; }

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
        const auto& t = ui_theme();
        const glm::vec4 rect{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const float radius = corner_radius(t.radius);

        glm::vec4 bg = tint_ * t.accent;
        bg.a = 0.10f;
        graphics::append_rounded_rect(list, rect, radius, bg);

        const float w = t.border;
        const glm::vec4 border = t.panel_border;
        graphics::Line top{rect.x, rect.y, rect.x + rect.z, rect.y, w, border};
        graphics::Line bottom{rect.x, rect.y + rect.w, rect.x + rect.z, rect.y + rect.w, w, border};
        graphics::Line left{rect.x, rect.y, rect.x, rect.y + rect.w, w, border};
        graphics::Line right{rect.x + rect.z, rect.y, rect.x + rect.z, rect.y + rect.w, w, border};
        top.append_to(list);
        bottom.append_to(list);
        left.append_to(list);
        right.append_to(list);

        glm::vec4 cross = t.panel_border;
        cross.a *= 0.6f;
        graphics::Line diag_a{rect.x, rect.y, rect.x + rect.z, rect.y + rect.w, w, cross};
        graphics::Line diag_b{rect.x + rect.z, rect.y, rect.x, rect.y + rect.w, w, cross};
        diag_a.append_to(list);
        diag_b.append_to(list);

        auto* atlas = font_atlas();
        if (atlas == nullptr) return;

        const std::string label = placeholder_text();
        const float size = t.small_text_size;
        const glm::vec2 text = atlas->measure(label, size);
        if (text.x > rect.z - 4.0f) return; // 放不下就不画，避免溢出

        list.push_clip(rect);
        atlas->append_text(list,
                           rect.x + (rect.z - text.x) * 0.5f,
                           rect.y + (rect.w - text.y) * 0.5f,
                           size,
                           label,
                           t.text_muted,
                           {rect.x, rect.y, rect.x + rect.z, rect.y + rect.w});
        list.pop_clip();
    }

private:
    std::string source_;
    std::string placeholder_;
    std::uint32_t texture_id_ = 0;
    Fit fit_ = Fit::Contain;
    glm::vec4 tint_{1.0f, 1.0f, 1.0f, 1.0f};
};
} // namespace xel::ui
