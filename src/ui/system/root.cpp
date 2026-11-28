#include "ui/system/root.hpp"
#include "ui/core/scrim.hpp"

#include <algorithm>

namespace
{
// 只做地址比较，不解引用 target：target 可能是已销毁的悬空指针。
bool widget_in_tree(xel::ui::Node* root, xel::ui::Node* target)
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
    for (xel::ui::Node* child : root->layer_paint_order()) {
        if (child->focusable()) return xel::ui::as_layout(child);
        if (xel::ui::Widget* w = first_focusable(xel::ui::as_layout(child))) return w;
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

void UIRoot::add(std::unique_ptr<Node> child)
{
    adopt(std::move(child));
}

void UIRoot::remove(Node* child)
{
    if (!child) return;
    for (std::size_t i = 0; i < child_count(); ++i) {
        if (child_at(i) != child) continue;
        // 先移入 trash_ 延后销毁（事件回调里可能还在使用该控件）
        remove_children_if([child](const std::unique_ptr<Node>& c) { return c.get() == child; });
        mark_layout_dirty();
        return;
    }
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

void UIRoot::add_overlay(std::unique_ptr<Node> child)
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
    // 主题可变性的单一入口：set_theme 推进本 context 的 revision，
    // 所有依赖主题的派生缓存（样式过渡）据此重建。
    context_storage_.set_theme(value);
    set_context(&context_storage_);
    // 度量缓存也要递归失效：主题会改变控件尺寸（行高/内边距/间距）。
    invalidate_measure_recursive();
    mark_layout_dirty();
}

void UIRoot::append_to(graphics::DrawList& list) const
{
    // 统一按 layer 升序 → z 升序绘制；overlay 不再单独在 children 之上。
    for (Node* widget : layer_paint_order()) {
        if (widget->visible()) widget->append_to(list);
    }
    if (is_debug()) append_debug_tree(list);
}

void UIRoot::layout_child(Widget& child)
{
    // 隐藏子项完全不参与布局（仍保留已有几何，重新可见时会被重排）。
    if (child.display() == Widget::Display::Hidden) return;    // 铺满父容器的控件（如模态遮罩）不做测量协商。
    if (child.fills_parent()) {
        child.arrange({bounds_.pos, bounds_.size});
        return;
    }
    // 顶层 Fill 模式：同时声明 fill_x/fill_y 的子项（顶层 Vbox 外壳）直接铺满 root，
    // 其余子项（窗口/弹层）保留原有坐标语义。
    if (child.fill_x() && child.fill_y()) {
        child.arrange({bounds_.pos, bounds_.size});
        return;
    }
    const glm::vec2 child_size = Widget::measure_child(child, bounds_.size);
    child.arrange({
        {bounds_.pos.x + child.x(), bounds_.pos.y + child.y()},
        child_size
    });
}

void UIRoot::layout()
{
    purge_trash();
    // 剔除悬空帧：模态窗口被父容器直接销毁（未走 release_modal）时，modal_frames_ 里会留野指针；
    // 同时清掉残留 scrim，避免界面被透明遮罩永久挡住。
    std::size_t keep = 0;
    for (std::size_t i = 0; i < modal_frames_.size(); ++i) {
        if (!widget_in_tree(this, modal_frames_[i].window)) {
            if (Widget* scrim = modal_frames_[i].scrim) {
                remove_children_if([scrim](const std::unique_ptr<Node>& c) {
                    return c.get() == scrim;
                });
            }
            continue;
        }
        modal_frames_[keep] = modal_frames_[i];
        ++keep;
    }
    modal_frames_.resize(keep);

    // 模态帧的 z 在 Modal 层内确定性分配：frame i → scrim 2i+1 / window 2i+2。
    // 这样 scrim 永远压在自己窗口之下、前一个模态窗口之上，嵌套模态不再靠插入顺序或 kOnTopZ 撞 z。
    for (std::size_t i = 0; i < modal_frames_.size(); ++i) {
        if (modal_frames_[i].scrim != nullptr) {
            modal_frames_[i].scrim->set_z_index(static_cast<int>(2 * i + 1));
        }
        if (modal_frames_[i].window != nullptr) {
            modal_frames_[i].window->set_z_index(static_cast<int>(2 * i + 2));
        }
    }
    // 整轮布局都在 LayoutPassScope 内：arrange 里的几何写入只失效缓存，不重新标记脏。
    // 轮次推进：measure 缓存以它为键，保证只在本轮内命中。
    advance_layout_epoch();
    LayoutPassScope layout_pass;
    for (auto& child : children()) layout_child(as_layout(*child));
    for (auto& overlay : overlays()) layout_child(as_layout(*overlay));
    // 收尾：清除整棵树的本轮脏标记（早退传播依赖它）。
    clear_layout_dirty_recursive();
}

void UIRoot::push_modal(Widget* window, bool with_scrim)
{
    if (window == nullptr || is_modal(window)) return;

    ModalFrame frame;
    frame.window = window;
    // 记录打开前的焦点快照，pop 时恢复。
    frame.prev_focus = focus_getter_ ? focus_getter_() : nullptr;

    if (with_scrim) {
        auto raw_scrim = std::make_unique<Scrim>();
        frame.scrim = raw_scrim.get();
        raw_scrim->set_z_index(window->z_index() - 1);
        add(std::move(raw_scrim));
    }
    modal_frames_.push_back(frame);
    mark_layout_dirty();

    // 初始焦点：焦点已在模态子树内则保留；否则 preferred_focus → 首个可聚焦 → 窗口自身。
    if (!focus_setter_) return;
    Widget* current = focus_getter_ ? focus_getter_() : nullptr;
    if (current != nullptr && widget_in_tree(window, current)) {
        return; // 已在子树内
    }
    Widget* target = as_layout(window->preferred_focus());
    if (target == nullptr || !widget_in_tree(window, target)) {
        target = first_focusable(window);
    }
    if (target == nullptr) target = window;
    focus_setter_(target);
}

void UIRoot::pop_modal(Widget* window)
{
    const auto it = std::find_if(modal_frames_.begin(), modal_frames_.end(),
                                 [window](const ModalFrame& f) { return f.window == window; });
    if (it == modal_frames_.end()) return;
    Widget* scrim = it->scrim;
    Widget* restore = it->prev_focus;
    modal_frames_.erase(it);
    if (scrim != nullptr) {
        remove_children_if([scrim](const std::unique_ptr<Node>& child) {
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
    return std::any_of(modal_frames_.begin(), modal_frames_.end(),
                       [window](const ModalFrame& f) { return f.window == window; });
}

glm::vec2 UIRoot::measure(glm::vec2 constraints)
{
    (void)constraints;
    return bounds_.size;
}

void UIRoot::arrange(Rect slot)
{
    set_geometry(slot);
    layout();
}
} // namespace xel::ui
