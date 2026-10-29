#include "root.hpp"
#include "scrim.hpp"

#include <algorithm>

namespace xel::ui
{
UIRoot::UIRoot(float x, float y, float width, float height)
{
    set_pos(x, y);
    set_size(width, height);
    set_context(&context_storage_);
}

void UIRoot::add(std::unique_ptr<Widget> child)
{
    adopt(std::move(child));
}

void UIRoot::add_overlay(std::unique_ptr<Widget> child)
{
    child->set_parent(this);
    child->set_context(context());
    overlays_.push_back(std::move(child));
    mark_layout_dirty();
}

void UIRoot::remove_overlay(Widget* overlay)
{
    if (!overlay) return;
    for (auto it = overlays_.begin(); it != overlays_.end(); ++it) {
        if (it->get() == overlay) {
            trash_.push_back(std::move(*it));
            overlays_.erase(it);
            mark_layout_dirty();
            return;
        }
    }
}

void UIRoot::purge_trash()
{
    trash_.clear();
}

void UIRoot::set_theme(const Theme& value)
{
    context_storage_.theme = value;
    // 主题变化后让所有控件的样式过渡缓存立即失效。
    bump_theme_revision();
    set_context(&context_storage_);
    mark_layout_dirty();
}

void UIRoot::append_to(graphics::DrawList& list) const
{
    // z-index 控制同层绘制顺序；overlays 始终在 children 之上。
    for (Widget* child : paint_order()) {
        if (child->visible()) child->append_to(list);
    }
    for (Widget* overlay : overlay_paint_order()) {
        if (overlay->visible()) overlay->append_to(list);
    }
    if (is_debug()) append_debug_tree(list);
}

void UIRoot::layout_child(Widget& child)
{
    // 铺满父容器的控件（如模态遮罩）不做测量协商。
    if (child.fills_parent()) {
        child.arrange({local_.pos, local_.size});
        return;
    }
    const glm::vec2 child_size = child.clamp_size(child.measure(local_.size));
    child.arrange({
        {local_.pos.x + child.x(), local_.pos.y + child.y()},
        child_size
    });
}

void UIRoot::layout()
{
    purge_trash();
    // 遮罩始终贴在对应窗口下方：窗口 raise 归一化过 z 也不会失配。
    for (std::size_t i = 0; i < modals_.size(); ++i) {
        if (modal_scrims_[i] != nullptr && modals_[i] != nullptr) {
            modal_scrims_[i]->set_z_index(modals_[i]->z_index() - 1);
        }
    }
    for (auto& child : children_) layout_child(*child);
    for (auto& overlay : overlays_) layout_child(*overlay);
}

void UIRoot::push_modal(Widget* window)
{
    if (window == nullptr || is_modal(window)) return;
    auto scrim = std::make_unique<Scrim>();
    Widget* raw = scrim.get();
    raw->set_z_index(window->z_index() - 1);
    add(std::move(scrim));
    modals_.push_back(window);
    modal_scrims_.push_back(raw);
    mark_layout_dirty();
}

void UIRoot::pop_modal(Widget* window)
{
    const auto it = std::find(modals_.begin(), modals_.end(), window);
    if (it == modals_.end()) return;
    const auto index = static_cast<std::size_t>(it - modals_.begin());
    Widget* scrim = modal_scrims_[index];
    modals_.erase(it);
    modal_scrims_.erase(modal_scrims_.begin() + static_cast<std::ptrdiff_t>(index));
    if (scrim != nullptr) {
        std::erase_if(children_, [scrim](const std::unique_ptr<Widget>& child) {
            return child.get() == scrim;
        });
    }
    mark_layout_dirty();
}

bool UIRoot::is_modal(Widget* window) const
{
    return std::find(modals_.begin(), modals_.end(), window) != modals_.end();
}

glm::vec2 UIRoot::measure(glm::vec2 constraints)
{
    (void)constraints;
    return local_.size;
}

void UIRoot::arrange(Rect slot)
{
    global_ = slot;
    set_pos(slot.pos);
    set_size(slot.size);
    layout();
}
} // namespace xel::ui
