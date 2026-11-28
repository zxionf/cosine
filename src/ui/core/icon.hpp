#pragma once

#include "graphics/box.hpp"
#include "graphics/sdf.hpp"
#include "graphics/sdf_atlas.hpp"
#include "ui/node/widget.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>

namespace xel::ui
{
// 图标：MTSDF 图集里的一个 cell，按目标尺寸任意缩放。
//
// 和 ui::Image 的区别：Image 走位图纹理槽，按像素拉伸会糊；
// 图标走 kind=3 的 SDF 采样 —— 同一份 96px cell 在 16~64px 都清晰，
// 因为覆盖率和缩放由 fragment 里的距离场决定，与图集分辨率无关。
//
// 图集无效 / 槽未上传 / 名字找不到时退化为描边方块，不会画黑块也不会崩。
class Icon final : public Widget
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::Image; }

    Icon(const graphics::SdfAtlas* atlas, int image_slot, std::string icon_name, float size = 20.0f)
    : atlas_{atlas}, slot_{image_slot}, name_{std::move(icon_name)}, size_{size}
    {
        set_preferred_size({size_, size_});
        set_size(size_, size_);
        index_ = atlas_ != nullptr ? atlas_->icon_index(name_) : -1;
    }

    void set_icon(std::string icon_name)
    {
        name_ = std::move(icon_name);
        index_ = atlas_ != nullptr ? atlas_->icon_index(name_) : -1;
        mark_layout_dirty();
    }
    const std::string& icon_name() const { return name_; }
    // 图集里是否能找到该图标（false 时画占位方块）。
    bool resolved() const { return index_ >= 0; }

    // 目标边长（正方形）。图集 cell 尺寸与它无关：SDF 负责无损缩放。
    void set_icon_size(float size)
    {
        size_ = std::max(size, 0.0f);
        set_preferred_size({size_, size_});
        mark_layout_dirty();
    }
    float icon_size() const { return size_; }

    void set_color(glm::vec4 color)
    {
        color_ = color;
        mark_layout_dirty();
    }
    glm::vec4 color() const { return color_; }

    glm::vec2 measure(glm::vec2 /*constraints*/) override { return {size_, size_}; }

    void append_to(graphics::DrawList& list) const override
    {
        // 保持正方形：非等比拉伸会把尖角拉歪，所以取短边居中。
        const float side = std::max(0.0f, std::min(bounds_.size.x, bounds_.size.y));
        if (side <= 0.0f) return;
        const float x = bounds_.pos.x + (bounds_.size.x - side) * 0.5f;
        const float y = bounds_.pos.y + (bounds_.size.y - side) * 0.5f;

        glm::vec4 uv{0.0f};
        if (atlas_ == nullptr || !atlas_->valid() || slot_ < 0 || index_ < 0 ||
            !atlas_->icon_uv(index_, uv)) {
            append_placeholder(list, {x, y, side, side});
            return;
        }
        graphics::append_sdf_icon(list, slot_, uv, {x, y, side, side}, color_,
                                  atlas_->px_range());
    }

    // 图标是装饰性叶子，本身不参与点击/焦点命中（交给宿主按钮）。
    bool hit_testable() const override { return false; }

private:
    void append_placeholder(graphics::DrawList& list, glm::vec4 rect) const
    {
        graphics::append_rounded_ring(list, rect, rect.z * 0.22f, 1.0f,
                                      {color_.r, color_.g, color_.b, color_.a * 0.6f});
        // 对角交叉线，明确表示「这里本该有个图标」。
        const float inset = rect.z * 0.28f;
        graphics::append_rounded_rect(list, {rect.x + inset, rect.y + rect.w * 0.5f - 0.5f,
                                             rect.z - inset * 2.0f, 1.0f},
                                      0.0f, {color_.r, color_.g, color_.b, color_.a * 0.4f});
    }

    const graphics::SdfAtlas* atlas_ = nullptr;
    int slot_ = -1;
    std::string name_;
    float size_ = 20.0f;
    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};
    int index_ = -1;
};
} // namespace xel::ui
