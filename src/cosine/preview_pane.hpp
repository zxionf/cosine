#pragma once

#include "ui/rect.hpp"
#include "ui/widget.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace xel::editor
{
// 预览区容器：主体（SceneView）占满剩余高度，底部固定高度的标尺。
// `Vbox` 没有弹性子项，而预览必须跟随窗口拉伸，所以这里手写纵向排布。
class PreviewPane : public xel::ui::Widget
{
public:
    PreviewPane(std::unique_ptr<xel::ui::Widget> main, std::unique_ptr<xel::ui::Widget> footer,
                float footer_height)
        : footer_height_(std::max(0.0f, footer_height))
    {
        children_.push_back(std::move(main));
        children_.push_back(std::move(footer));
        main_ = children_[0].get();
        footer_ = children_[1].get();
    }

    xel::ui::Widget* main() const { return main_; }
    xel::ui::Widget* footer() const { return footer_; }

    const std::vector<std::unique_ptr<xel::ui::Widget>>& children() const override
    {
        return children_;
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        const float width =
            std::isfinite(constraints.x) && constraints.x > 0.0f ? constraints.x : 320.0f;
        const float height =
            std::isfinite(constraints.y) && constraints.y > 0.0f ? constraints.y : 240.0f;
        return clamp_size({width, height});
    }

    void arrange(xel::ui::Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);

        const float footer_h = std::min(footer_height_, slot.size.y);
        const float main_h = std::max(0.0f, slot.size.y - footer_h);
        main_->arrange({{slot.pos.x, slot.pos.y}, {slot.size.x, main_h}});
        footer_->arrange(
            {{slot.pos.x, slot.pos.y + main_h}, {slot.size.x, footer_h}});
    }

    void append_to(xel::graphics::DrawList& list) const override
    {
        main_->append_to(list);
        footer_->append_to(list);
    }

private:
    std::vector<std::unique_ptr<xel::ui::Widget>> children_;
    xel::ui::Widget* main_ = nullptr;
    xel::ui::Widget* footer_ = nullptr;
    float footer_height_ = 0.0f;
};
} // namespace xel::editor
