#include "debug_window.hpp"

#include "graphics/text_debug.hpp"
#include "ui/composites/dropdown.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/slider.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/tab_bar.hpp"
#include "ui/core/text_input.hpp"
#include "ui/views/file_browser.hpp"
#include "ui/views/scene_view.hpp"
#include "ui/views/timeline_view.hpp"
#include "ui/views/tree_view.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <utility>

namespace xel::editor
{
namespace
{
bool is_descendant(const ui::Widget* widget, const ui::Widget* ancestor)
{
    if (widget == nullptr || ancestor == nullptr) return false;
    for (const ui::Widget* w = widget; w != nullptr; w = w->parent()) {
        if (w == ancestor) return true;
    }
    return false;
}

bool widget_in_tree(const ui::Widget* root, const ui::Widget* target)
{
    if (root == nullptr || target == nullptr) return false;
    if (root == target) return true;
    for (const auto& child : root->children()) {
        if (widget_in_tree(child.get(), target)) return true;
    }
    for (const auto& overlay : root->overlays()) {
        if (widget_in_tree(overlay.get(), target)) return true;
    }
    return false;
}

std::string widget_name(const ui::Widget* widget)
{
    if (widget == nullptr) return "-";
    if (dynamic_cast<const ui::Label*>(widget) != nullptr) return "Label";
    if (dynamic_cast<const ui::Button*>(widget) != nullptr) return "Button";
    if (dynamic_cast<const ui::Checkbox*>(widget) != nullptr) return "Checkbox";
    if (dynamic_cast<const ui::Dropdown*>(widget) != nullptr) return "Dropdown";
    if (dynamic_cast<const ui::DragFloat*>(widget) != nullptr) return "DragFloat";
    if (dynamic_cast<const ui::Slider*>(widget) != nullptr) return "Slider";
    if (dynamic_cast<const ui::Spacer*>(widget) != nullptr) return "Spacer";
    if (dynamic_cast<const ui::ColorRect*>(widget) != nullptr) return "ColorRect";
    if (dynamic_cast<const ui::TextInput*>(widget) != nullptr) return "TextInput";
    if (dynamic_cast<const ui::FileBrowser*>(widget) != nullptr) return "FileBrowser";
    if (dynamic_cast<const ui::ScrollPanel*>(widget) != nullptr) return "ScrollPanel";
    if (dynamic_cast<const ui::TabBar*>(widget) != nullptr) return "TabBar";
    if (dynamic_cast<const ui::TabView*>(widget) != nullptr) return "TabView";
    if (dynamic_cast<const ui::Window*>(widget) != nullptr) return "Window";
    if (dynamic_cast<const ui::Panel*>(widget) != nullptr) return "Panel";
    if (dynamic_cast<const ui::Hbox*>(widget) != nullptr) return "Hbox";
    if (dynamic_cast<const ui::Vbox*>(widget) != nullptr) return "Vbox";
    if (dynamic_cast<const ui::SceneView*>(widget) != nullptr) return "SceneView";
    if (dynamic_cast<const ui::TreeView*>(widget) != nullptr) return "TreeView";
    if (dynamic_cast<const ui::TimelineView*>(widget) != nullptr) return "TimelineView";
    return "Widget";
}

void set_debug_recursive(ui::Widget* widget, bool enabled)
{
    if (widget == nullptr) return;
    widget->set_debug(enabled);
    for (const auto& child : widget->children()) set_debug_recursive(child.get(), enabled);
    for (const auto& overlay : widget->overlays()) set_debug_recursive(overlay.get(), enabled);
}

void apply_text_debug_recursive(ui::Widget* widget, const ui::Widget* skip,
                                const graphics::TextDebugOptions& options)
{
    if (widget == nullptr || widget == skip) return;
    if (auto* label = dynamic_cast<ui::Label*>(widget)) {
        label->set_text_debug(options);
    } else if (auto* button = dynamic_cast<ui::Button*>(widget)) {
        button->set_text_debug(options);
    } else if (auto* window = dynamic_cast<ui::Window*>(widget)) {
        window->set_text_debug(options);
    }
    for (const auto& child : widget->children()) {
        apply_text_debug_recursive(child.get(), skip, options);
    }
    for (const auto& overlay : widget->overlays()) {
        apply_text_debug_recursive(overlay.get(), skip, options);
    }
}

void notify_layout(ui::Widget* widget)
{
    if (widget == nullptr) return;
    widget->mark_layout_dirty();
    if (auto* parent = widget->parent()) parent->mark_layout_dirty();
}
} // namespace

DebugWindow::DebugWindow(ui::UIRoot& root) : root_(&root)
{
    auto window = std::make_unique<ui::Window>("Debug Window", glm::vec2{760.0f, 120.0f},
                                               glm::vec2{460.0f, 540.0f});
    window->set_min_size({360.0f, 320.0f});
    window->set_always_on_top(true);
    window_ = window.get();
    root.add(std::move(window));
    window_->set_visible(false);
    build();
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::show()
{
    if (window_ != nullptr) window_->set_visible(true);
}

void DebugWindow::hide()
{
    if (window_ != nullptr) window_->set_visible(false);
}

bool DebugWindow::visible() const
{
    return window_ != nullptr && window_->visible();
}

bool DebugWindow::is_own(const ui::Widget* widget) const
{
    return is_descendant(widget, window_);
}

void DebugWindow::build()
{
    ui::Vbox* content = window_->content();
    content->set_padding(4.0f);
    content->set_spacing(0.0f);
    content->set_cross_align(ui::Align::top_left());

    auto view = std::make_unique<ui::TabView>();
    view->set_size_percent({1.0f, 0.0f});
    view->set_flex_grow(1.0f);
    view->set_min_size({0.0f, 0.0f});
    view->set_max_size({10000.0f, 10000.0f});
    ui::TabView* view_ptr = view.get();
    content->add(std::move(view));
    // Window::add 会经 set_context 重新套 theme padding，这里覆盖回 0。
    view_ptr->set_padding(0.0f);

    auto widget_page = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    widget_page->set_padding(0.0f);
    widget_page->set_min_size({0.0f, 0.0f});
    widget_page->set_max_size({10000.0f, 10000.0f});
    build_widget_page(*widget_page);
    view_ptr->add_page("Widget", std::move(widget_page));

    auto font_page = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    font_page->set_padding(0.0f);
    font_page->set_min_size({0.0f, 0.0f});
    font_page->set_max_size({10000.0f, 10000.0f});
    build_font_page(*font_page);
    view_ptr->add_page("Font", std::move(font_page));

    view_ptr->set_selected(0);
}

void DebugWindow::build_widget_page(ui::ScrollPanel& page)
{
    ui::Vbox* inner = page.content();
    inner->set_padding(0.0f);
    inner->set_spacing(6.0f);
    inner->set_cross_align(ui::Align::top_left());

    auto target = std::make_unique<ui::Label>("target: -", 22.0f);
    target_label_ = target.get();
    inner->add(std::move(target));

    auto info = std::make_unique<ui::Label>("hover a widget", 22.0f);
    info_label_ = info.get();
    inner->add(std::move(info));

    auto add_drag = [this, inner](const std::string& name, float value,
                                  std::function<void(float)> on_change) -> ui::DragFloat* {
        auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 34.0f);
        row->set_padding(0.0f);
        row->set_spacing(6.0f);
        row->set_cross_align(ui::Align::center_left());
        row->set_size_percent({1.0f, 0.0f});
        row->add(std::make_unique<ui::Label>(name, 22.0f));
        auto drag = std::make_unique<ui::DragFloat>("", 100.0f);
        drag->set_range(-1.0e5f, 1.0e5f);
        drag->set_value(value);
        drag->set_on_change(std::move(on_change));
        auto* ptr = drag.get();
        row->add(std::move(drag));
        inner->add(std::move(row));
        return ptr;
    };

    pos_x_ = add_drag("Pos X", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        auto pos = target_->local_bounds().pos;
        pos.x = value;
        target_->set_pos(pos);
        notify_layout(target_);
    });
    pos_y_ = add_drag("Pos Y", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        auto pos = target_->local_bounds().pos;
        pos.y = value;
        target_->set_pos(pos);
        notify_layout(target_);
    });
    size_w_ = add_drag("Size W", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        auto size = target_->local_bounds().size;
        size.x = value;
        target_->set_size(size);
        notify_layout(target_);
    });
    size_h_ = add_drag("Size H", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        auto size = target_->local_bounds().size;
        size.y = value;
        target_->set_size(size);
        notify_layout(target_);
    });
    z_ = add_drag("Z", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        target_->set_z_index(static_cast<int>(std::lround(value)));
        notify_layout(target_);
    });
    flex_ = add_drag("Flex", 0.0f, [this](float value) {
        if (updating_ || target_ == nullptr) return;
        target_->set_flex_grow(std::max(0.0f, value));
        notify_layout(target_);
    });

    auto visible = std::make_unique<ui::Checkbox>("Visible", true, [this](bool checked) {
        if (updating_ || target_ == nullptr) return;
        target_->set_visible(checked);
    });
    visible_ = visible.get();
    inner->add(std::move(visible));

    auto absolute = std::make_unique<ui::Checkbox>("Absolute", false, [this](bool checked) {
        if (updating_ || target_ == nullptr) return;
        target_->set_absolute(checked);
        notify_layout(target_);
    });
    absolute_ = absolute.get();
    inner->add(std::move(absolute));

    auto debug = std::make_unique<ui::Checkbox>("Debug", false, [this](bool checked) {
        if (updating_ || target_ == nullptr) return;
        target_->set_debug(checked);
        if (root_ != nullptr) root_->set_debug(true);
    });
    debug_ = debug.get();
    inner->add(std::move(debug));

    auto debug_children = std::make_unique<ui::Checkbox>(
        "Debug children", false, [this](bool checked) {
            if (target_ == nullptr) return;
            set_debug_recursive(target_, checked);
            if (root_ != nullptr) root_->set_debug(true);
        });
    debug_children_ = debug_children.get();
    inner->add(std::move(debug_children));

    auto follow = std::make_unique<ui::Checkbox>("Follow hover", follow_, [this](bool checked) {
        follow_ = checked;
    });
    follow_box_ = follow.get();
    inner->add(std::move(follow));

    inner->add(std::make_unique<ui::Button>("Clear", [this] {
        target_ = nullptr;
        rebuild_children();
        update_props();
    }));

    auto children = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    children->set_spacing(4.0f);
    children->set_cross_align(ui::Align::top_left());
    children_box_ = children.get();
    inner->add(std::move(children));

    rebuild_children();
}

void DebugWindow::build_font_page(ui::ScrollPanel& page)
{
    ui::Vbox* inner = page.content();
    inner->set_padding(10.0f);
    inner->set_spacing(8.0f);
    inner->set_cross_align(ui::Align::top_left());

    inner->add(std::make_unique<ui::Label>("Text debug", 22.0f));
    inner->add(std::make_unique<ui::Label>(
        "字形框 / 行框 / 基线，作用于当前控件树", 22.0f));

    inner->add(std::make_unique<ui::Button>("Toggle font debug", [this] {
        font_debug_ = !font_debug_;
        apply_text_debug(font_debug_, false);
    }));
    inner->add(std::make_unique<ui::Button>("Glyph boxes only", [this] {
        font_debug_ = true;
        apply_text_debug(true, true);
    }));
    inner->add(std::make_unique<ui::Label>(
        "新建控件不会自动继承；Debug Window 自身不绘制", 22.0f));
}

void DebugWindow::apply_text_debug(bool enabled, bool glyph_only)
{
    graphics::TextDebugOptions options;
    if (enabled) {
        options.glyph_boxes = true;
        if (!glyph_only) {
            options.line_box = true;
            options.baseline = true;
        }
    }
    apply_text_debug_recursive(root_, window_, options);
}

void DebugWindow::rebuild_children()
{
    if (children_box_ == nullptr) return;
    children_box_->clear_children();
    if (target_ == nullptr) return;

    if (auto* parent = target_->parent()) {
        children_box_->add(std::make_unique<ui::Button>(
            "^ parent: " + widget_name(parent), [this, parent] {
                target_ = parent;
                follow_ = false;
                if (follow_box_ != nullptr) follow_box_->set_checked(false);
                rebuild_children();
            }));
    }

    children_box_->add(std::make_unique<ui::Label>("children:", 22.0f));
    for (const auto& child : target_->children()) {
        auto* ptr = child.get();
        children_box_->add(std::make_unique<ui::Button>(
            widget_name(ptr), [this, ptr] {
                target_ = ptr;
                follow_ = false;
                if (follow_box_ != nullptr) follow_box_->set_checked(false);
                rebuild_children();
            }));
    }
}

void DebugWindow::update_props()
{
    updating_ = true;
    const bool has = target_ != nullptr;
    const ui::Rect bounds = has ? target_->local_bounds() : ui::Rect{};

    if (pos_x_ != nullptr) pos_x_->set_value(bounds.pos.x);
    if (pos_y_ != nullptr) pos_y_->set_value(bounds.pos.y);
    if (size_w_ != nullptr) size_w_->set_value(bounds.size.x);
    if (size_h_ != nullptr) size_h_->set_value(bounds.size.y);
    if (z_ != nullptr) z_->set_value(has ? static_cast<float>(target_->z_index()) : 0.0f);
    if (flex_ != nullptr) flex_->set_value(has ? target_->flex_grow() : 0.0f);
    if (visible_ != nullptr) visible_->set_checked(has && target_->visible());
    if (absolute_ != nullptr) absolute_->set_checked(has && target_->absolute());
    if (debug_ != nullptr) debug_->set_checked(has && target_->is_debug());
    updating_ = false;
}

void DebugWindow::update(const ui::UISystem& uisys, float dt)
{
    if (window_ == nullptr || !window_->visible()) return;

    // 目标被销毁、移出 root 或指向调试窗口自身时清空。
    if (target_ != nullptr && (!widget_in_tree(root_, target_) || is_own(target_))) {
        target_ = nullptr;
        rebuild_children();
    }

    hovered_ = nullptr;
    const ui::Widget* hovered = uisys.hovered();
    if (hovered != nullptr && hovered != root_ && !is_own(hovered)) {
        hovered_ = const_cast<ui::Widget*>(hovered);
        if (follow_) {
            if (hovered_ == hover_candidate_) {
                hover_time_ += dt;
            } else {
                hover_candidate_ = hovered_;
                hover_time_ = 0.0f;
            }
            if (hover_time_ >= 0.12f && target_ != hovered_) {
                target_ = hovered_;
                rebuild_children();
            }
        }
    } else {
        hover_candidate_ = nullptr;
        hover_time_ = 0.0f;
    }

    if (target_label_ != nullptr) {
        target_label_->set_text("target: " + widget_name(target_));
    }
    if (info_label_ != nullptr) {
        if (target_ == nullptr) {
            info_label_->set_text("hover a widget");
        } else {
            const auto& bounds = target_->global_bounds();
            info_label_->set_text(std::format(
                "pos {:.0f},{:.0f}  size {:.0f}x{:.0f}  visible {}  z {}  children {}",
                bounds.pos.x, bounds.pos.y, bounds.size.x, bounds.size.y,
                target_->visible() ? "yes" : "no", target_->z_index(),
                target_->children().size()));
        }
    }
    update_props();
}
} // namespace xel::editor
