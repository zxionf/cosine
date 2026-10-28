#pragma once

#include "widget.hpp"
#include "../anim/scene.hpp"
#include "../graphics/rectangle.hpp"
#include "../graphics/sprite.hpp"

#include <algorithm>
#include <cmath>

namespace xel::ui
{
// 把 scene::Scene 渲染成 DrawList 指令的控件。
// 目前支持：Group（仅 transform）、Shape（矩形 / 椭圆）、Text、Image（纹理 / 色块回退）。
// rotation 作用于矩形、旋转四边形；椭圆与文字暂不参与旋转。
class SceneView : public Widget
{
public:
    explicit SceneView(scene::Scene* scene = nullptr) : scene_(scene) {}

    void set_scene(scene::Scene* scene)
    {
        scene_ = scene;
        mark_layout_dirty();
    }
    scene::Scene* scene() const { return scene_; }

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
        if (scene_ == nullptr) return;
        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        draw_node(list, scene_->root(), {0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f);
        list.pop_clip();
    }

private:
    // parent_pos / parent_scale 是场景坐标系下的父级累积变换。
    // 绕节点中心旋转的矩形：两个三角形，与 Rectangle 同样的预乘 alpha 约定。
    static void append_rotated_rect(graphics::DrawList& list, glm::vec2 top_left, glm::vec2 size,
                                    float angle, glm::vec4 color)
    {
        const glm::vec2 white = graphics::active_font_atlas()
                                    ? graphics::active_font_atlas()->white_uv()
                                    : glm::vec2{0.0f};
        append_rotated_quad(list, top_left, size, angle, color,
                            static_cast<uint32_t>(graphics::TextureSlot::Texture),
                            glm::vec4{white.x, white.y, white.x, white.y});
    }

    // 旋转四边形：槽与 UV 可指定，图片用它；纯色传字体图集的白 uv。
    static void append_rotated_quad(graphics::DrawList& list, glm::vec2 top_left, glm::vec2 size,
                                    float angle, glm::vec4 color, uint32_t slot, glm::vec4 uv)
    {
        list.set_texture_slot(slot);
        auto& batch = list.batch();
        const glm::vec4 premultiplied{color.r * color.a, color.g * color.a, color.b * color.a,
                                      color.a};
        const glm::vec2 center{top_left.x + size.x * 0.5f, top_left.y + size.y * 0.5f};
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        const glm::vec2 half{size.x * 0.5f, size.y * 0.5f};
        auto corner = [&](float sx, float sy) {
            const glm::vec2 v{sx * half.x, sy * half.y};
            return glm::vec2{center.x + v.x * c - v.y * s, center.y + v.x * s + v.y * c};
        };
        const glm::vec2 p0 = corner(-1.0f, -1.0f);
        const glm::vec2 p1 = corner(1.0f, -1.0f);
        const glm::vec2 p2 = corner(1.0f, 1.0f);
        const glm::vec2 p3 = corner(-1.0f, 1.0f);

        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.vertices.push_back({p0, premultiplied, {uv.x, uv.y}});
        batch.vertices.push_back({p1, premultiplied, {uv.z, uv.y}});
        batch.vertices.push_back({p2, premultiplied, {uv.z, uv.w}});
        batch.vertices.push_back({p3, premultiplied, {uv.x, uv.w}});
        batch.indices.insert(batch.indices.end(), {
            base, base + 1, base + 2, base + 2, base + 3, base
        });
    }

    void draw_node(graphics::DrawList& list, scene::NodeId id,
                   glm::vec2 parent_pos, glm::vec2 parent_scale, float parent_rotation) const
    {
        const scene::Node* node = scene_->find(id);
        if (node == nullptr || !node->visible) return;

        const glm::vec2 scale = parent_scale * node->transform.scale;
        const glm::vec2 local = node->transform.position * parent_scale;
        // 父空间旋转：子节点偏移按父旋转后的坐标系定位。
        const float pc = std::cos(parent_rotation);
        const float ps = std::sin(parent_rotation);
        const glm::vec2 pos{parent_pos.x + local.x * pc - local.y * ps,
                            parent_pos.y + local.x * ps + local.y * pc};
        const float rotation = parent_rotation + node->transform.rotation;
        const glm::vec2 screen{global_.pos.x + pos.x, global_.pos.y + pos.y};

        switch (node->type) {
            case scene::NodeType::Shape: {
                const glm::vec2 size{node->size.x * scale.x, node->size.y * scale.y};
                if (node->ellipse) {
                    // 椭圆暂不参与旋转（位图圆角不支旋转，M6.2 只保证矩形）。
                    graphics::append_rounded_rect(
                        list, {screen.x, screen.y, size.x, size.y},
                        std::min(size.x, size.y) * 0.5f, node->color);
                } else if (std::fabs(rotation) < 1.0e-4f) {
                    graphics::Rectangle{screen.x, screen.y, size.x, size.y, node->color}
                        .append_to(list);
                } else {
                    append_rotated_rect(list, screen, size, rotation, node->color);
                }
                break;
            }
            case scene::NodeType::Text: {
                if (auto* atlas = font_atlas(); atlas != nullptr && !node->text.empty()) {
                    // 字形不旋转：只跟着旋转后的锚点定位。
                    atlas->append_text(
                        list, screen.x, screen.y, node->text_size * scale.y, node->text,
                        node->color,
                        {global_.pos.x, global_.pos.y,
                         global_.pos.x + global_.size.x, global_.pos.y + global_.size.y});
                }
                break;
            }
            case scene::NodeType::Image: {
                const glm::vec2 size{node->size.x * scale.x, node->size.y * scale.y};
                const bool rotated = std::fabs(rotation) >= 1.0e-4f;
                if (node->texture_id == 0) {
                    // 未绑定纹理：回退成色块，与旧行为一致（不显示占位图，避免误认）。
                    if (rotated) {
                        append_rotated_rect(list, screen, size, rotation, node->color);
                    } else {
                        graphics::Rectangle{screen.x, screen.y, size.x, size.y, node->color}
                            .append_to(list);
                    }
                    break;
                }

                // texture_id 是「图片槽索引 + 1」，换回槽号。
                const uint32_t slot = graphics::image_slot(node->texture_id - 1);
                const glm::vec4 uv{0.0f, 0.0f, 1.0f, 1.0f};
                if (rotated) {
                    append_rotated_quad(list, screen, size, rotation, node->color, slot, uv);
                } else {
                    graphics::Sprite{screen.x, screen.y, size.x, size.y, slot, node->color, uv}
                        .append_to(list);
                }
                break;
            }
            case scene::NodeType::Camera:
            case scene::NodeType::Group:
                break;
        }

        for (scene::NodeId child : node->children) {
            draw_node(list, child, pos, scale, rotation);
        }
    }

    scene::Scene* scene_ = nullptr;
};
} // namespace xel::ui
