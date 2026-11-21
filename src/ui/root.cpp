#include "ui/root.hpp"
#include "ui/core/scrim.hpp"

#include <algorithm>

namespace
{
// 只做地址比较，不解引用 target：target 可能是已销毁的悬空指针。
bool widget_in_tree(xel::ui::Widget* root, xel::ui::Widget* target)
{
    if (!root || !target) return false;
    if (root == target) return true;
    for (const auto& child : root->children()) {
        if (widget_in_tree(child.get(), target)) return true;
    }
    for (const auto& overlay : root->overlays()) {
        if (widget_in_tree(overlay.get(), target)) return true;
    }
    return false;
}

// 树序第一个可聚焦的子孙控件（不含 root 自身）。
xel::ui::Widget* first_focusable(xel::ui::Widget* root)
{
    if (!root) return nullptr;
    for (xel::ui::Widget* child : root->layer_paint_order()) {
        if (child->focusable()) return child;
        if (xel::ui::Widget* w = first_focusable(child)) return w;
    }
    return nullptr;
}
} // namespace

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

void UIRoot::set_focus_hooks(FocusGetter getter, FocusSetter setter)
{
    focus_getter_ = std::move(getter);
    focus_setter_ = std::move(setter);
}

void UIRoot::clear_focus_hooks()
{
    focus_getter_ = {};
    focus_setter_ = {};
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
    // 统一按 layer 升序 → z 升序绘制；overlay 不再单独在 children 之上。
    for (Widget* widget : layer_paint_order()) {
        if (widget->visible()) widget->append_to(list);
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
    // 剔除悬空帧：模态窗口被父容器直接销毁（未走 release_modal）时，modals_ 里会留野指针；
    // 同时清掉残留 scrim，避免界面被透明遮罩永久挡住。
    std::size_t keep = 0;
    for (std::size_t i = 0; i < modals_.size(); ++i) {
        if (!widget_in_tree(this, modals_[i])) {
            if (Widget* scrim = modal_scrims_[i]) {
                std::erase_if(children_, [scrim](const std::unique_ptr<Widget>& c) {
                    return c.get() == scrim;
                });
            }
            continue;
        }
        modals_[keep] = modals_[i];
        modal_scrims_[keep] = modal_scrims_[i];
        modal_focus_stack_[keep] = modal_focus_stack_[i];
        ++keep;
    }
    modals_.resize(keep);
    modal_scrims_.resize(keep);
    modal_focus_stack_.resize(keep);

    // 模态帧的 z 在 Modal 层内确定性分配：frame i → scrim 2i+1 / window 2i+2。
    // 这样 scrim 永远压在自己窗口之下、前一个模态窗口之上，嵌套模态不再靠插入顺序或 kOnTopZ 撞 z。
    for (std::size_t i = 0; i < modals_.size(); ++i) {
        if (modal_scrims_[i] != nullptr) {
            modal_scrims_[i]->set_z_index(static_cast<int>(2 * i + 1));
        }
        if (modals_[i] != nullptr) {
            modals_[i]->set_z_index(static_cast<int>(2 * i + 2));
        }
    }
    for (auto& child : children_) layout_child(*child);
    for (auto& overlay : overlays_) layout_child(*overlay);
}

void UIRoot::push_modal(Widget* window, bool with_scrim)
{
    if (window == nullptr || is_modal(window)) return;

    // 记录打开前的焦点快照，pop 时恢复。
    const Widget* prev_focus = focus_getter_ ? focus_getter_() : nullptr;
    modal_focus_stack_.push_back(const_cast<Widget*>(prev_focus));

    Widget* scrim = nullptr;
    if (with_scrim) {
        auto raw_scrim = std::make_unique<Scrim>();
        scrim = raw_scrim.get();
        raw_scrim->set_z_index(window->z_index() - 1);
        add(std::move(raw_scrim));
    }
    modal_scrims_.push_back(scrim);
    modals_.push_back(window);
    mark_layout_dirty();

    // 初始焦点：焦点已在模态子树内则保留；否则 preferred_focus → 首个可聚焦 → 窗口自身。
    if (!focus_setter_) return;
    Widget* current = focus_getter_ ? focus_getter_() : nullptr;
    if (current != nullptr && widget_in_tree(window, current)) {
        return; // 已在子树内
    }
    Widget* target = window->preferred_focus();
    if (target == nullptr || !widget_in_tree(window, target)) {
        target = first_focusable(window);
    }
    if (target == nullptr) target = window;
    focus_setter_(target);
}

void UIRoot::pop_modal(Widget* window)
{
    const auto it = std::find(modals_.begin(), modals_.end(), window);
    if (it == modals_.end()) return;
    const auto index = static_cast<std::size_t>(it - modals_.begin());
    Widget* scrim = modal_scrims_[index];
    Widget* restore = modal_focus_stack_[index];
    modals_.erase(it);
    modal_scrims_.erase(modal_scrims_.begin() + static_cast<std::ptrdiff_t>(index));
    modal_focus_stack_.erase(modal_focus_stack_.begin() + static_cast<std::ptrdiff_t>(index));
    if (scrim != nullptr) {
        std::erase_if(children_, [scrim](const std::unique_ptr<Widget>& child) {
            return child.get() == scrim;
        });
    }
    mark_layout_dirty();

    // 恢复焦点：快照仍在树中且不在被弹出子树内才恢复，否则清空。
    if (!focus_setter_) return;
    if (restore != nullptr && widget_in_tree(this, restore) && !widget_in_tree(window, restore)) {
        focus_setter_(restore);
    } else {
        focus_setter_(nullptr);
    }
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
