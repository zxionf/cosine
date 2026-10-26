#pragma once

#include "widget.hpp"
#include "../anim/scene.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>

namespace xel::ui
{
// 把 scene::Scene 渲染成 DrawList 指令的控件。
// 目前支持：Group（仅 transform）、Shape（矩形 / 椭圆）、Text、Image（占位色块）。
// rotation 暂不参与（M4 后续）；Image 的真实纹理等 M6 资源管线。
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
        draw_node(list, scene_->root(), {0.0f, 0.0f}, {1.0f, 1.0f});
        list.pop_clip();
    }

private:
    // parent_pos / parent_scale 是场景坐标系下的父级累积变换。
    void draw_node(graphics::DrawList& list, scene::NodeId id,
                   glm::vec2 parent_pos, glm::vec2 parent_scale) const
    {
        const scene::Node* node = scene_->find(id);
        if (node == nullptr || !node->visible) return;

        const glm::vec2 scale = parent_scale * node->transform.scale;
        const glm::vec2 pos = parent_pos + node->transform.position * parent_scale;
        const glm::vec2 screen{global_.pos.x + pos.x, global_.pos.y + pos.y};

        switch (node->type) {
            case scene::NodeType::Shape: {
                const glm::vec2 size{node->size.x * scale.x, node->size.y * scale.y};
                if (node->ellipse) {
                    graphics::append_rounded_rect(
                        list, {screen.x, screen.y, size.x, size.y},
                        std::min(size.x, size.y) * 0.5f, node->color);
                } else {
                    graphics::Rectangle{screen.x, screen.y, size.x, size.y, node->color}
                        .append_to(list);
                }
                break;
            }
            case scene::NodeType::Text: {
                if (auto* atlas = font_atlas(); atlas != nullptr && !node->text.empty()) {
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
                graphics::Rectangle{screen.x, screen.y, size.x, size.y, node->color}
                    .append_to(list);
                break;
            }
            case scene::NodeType::Camera:
            case scene::NodeType::Group:
                break;
        }

        for (scene::NodeId child : node->children) {
            draw_node(list, child, pos, scale);
        }
    }

    scene::Scene* scene_ = nullptr;
};
} // namespace xel::ui
