#include "editor.hpp"

#include "anim/easing.hpp"
#include "anim/property.hpp"
#include "ui/button.hpp"
#include "ui/hbox.hpp"
#include "ui/theme.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

namespace xel::editor
{
namespace
{
namespace ns = xel::scene;
namespace prop = xel::anim::prop;

constexpr float kMenuHeight = 26.0f;
constexpr float kToolbarHeight = 40.0f;
constexpr float kTreeWidth = 240.0f;
constexpr float kPropertyWidth = 300.0f;
constexpr float kTimelineRatio = 0.30f;
constexpr float kStatusHeight = 22.0f;

// 固定尺寸 + 固定位置：UIRoot 的自由画布布局会尊重子项的 pos/size。
void place(ui::Widget* widget, float x, float y, float width, float height)
{
    if (widget == nullptr) return;
    const glm::vec2 size{width, height};
    widget->set_min_size(size);
    widget->set_max_size(size);
    widget->set_size(size);
    widget->set_pos(x, y);
    widget->mark_layout_dirty();
}
} // namespace

Editor::Editor(ui::UIRoot& root, ui::UISystem& uisys) : root_(&root), uisys_(&uisys)
{
    build_demo_content();
    build();
    apply_theme();
    layout();
}

void Editor::build()
{
    build_menu();
    build_toolbar();
    build_panels();
}

void Editor::build_menu()
{
    auto menu = std::make_unique<ui::MenuBar>();
    menu_ = menu.get();

    menu->add_menu("File", {
        ui::MenuItem::make("New", [this] { set_status("new project: pending M6.4b"); }, "Ctrl+N"),
        ui::MenuItem::make("Open...", [this] { set_status("open: pending M6.5"); }, "Ctrl+O"),
        ui::MenuItem::make("Save...", [this] { set_status("save: pending M6.5"); }, "Ctrl+S"),
        ui::MenuItem::separator_item(),
        ui::MenuItem::make("Exit", [this] { set_status("close the window to exit"); }, "Ctrl+Q"),
    });

    menu->add_menu("Edit", {
        ui::MenuItem::make("Undo", [this] { set_status("undo: pending M6.4b"); }, "Ctrl+Z"),
        ui::MenuItem::make("Redo", [this] { set_status("redo: pending M6.4b"); }, "Ctrl+Y"),
    });

    menu->add_menu("View", {
        ui::MenuItem::make("Experimental rounded style",
                           [this] { toggle_rounded_style(); }),
    });

    menu->add_menu("Playback", {
        ui::MenuItem::make("Play", [this] { set_status("play: pending M6.4b"); }, "Space"),
        ui::MenuItem::make("Reset", [this] {
            time_ = 0.0f;
            set_status("time reset");
        }),
    });

    root_->add(std::move(menu));
}

void Editor::build_toolbar()
{
    auto toolbar = std::make_unique<ui::Hbox>(0.0f, 0.0f, 520.0f, kToolbarHeight);
    toolbar->set_spacing(6.0f);
    toolbar_ = toolbar.get();

    auto add = [&toolbar](std::string label, std::function<void()> action) {
        toolbar->add(std::make_unique<ui::Button>(std::move(label), std::move(action)));
    };
    add("Play", [this] { set_status("play: pending M6.4b"); });
    add("Reset", [this] {
        time_ = 0.0f;
        set_status("time reset");
    });
    add("Add Shape", [this] { add_shape_node(); });
    add("Style", [this] { toggle_rounded_style(); });

    root_->add(std::move(toolbar));
}

void Editor::build_panels()
{
    auto tree = std::make_unique<ui::TreeView>(kTreeWidth, 300.0f);
    scene_tree_ = tree.get();
    tree->set_on_select([this](const std::vector<int>&) { refresh_property_panel(); });
    root_->add(std::move(tree));

    auto preview = std::make_unique<ui::SceneView>(&project_.scene);
    preview_ = preview.get();
    root_->add(std::move(preview));

    auto property = std::make_unique<ui::Panel>(0.0f, 0.0f, kPropertyWidth, 400.0f);
    property_panel_ = property.get();
    auto property_label = std::make_unique<ui::Label>("Properties (M6.2)", 13.0f);
    property_label_ = property_label.get();
    property->add(std::move(property_label));
    root_->add(std::move(property));

    auto timeline = std::make_unique<ui::Panel>(0.0f, 0.0f, 800.0f, 200.0f);
    timeline_panel_ = timeline.get();
    auto timeline_label = std::make_unique<ui::Label>(
        "Timeline placeholder (M6.3): ruler, tracks, keyframes", 13.0f);
    timeline_label_ = timeline_label.get();
    timeline->add(std::move(timeline_label));
    root_->add(std::move(timeline));

    auto status = std::make_unique<ui::Label>("ready", 12.0f);
    status_ = status.get();
    root_->add(std::move(status));

    refresh_scene_tree();
}

void Editor::build_demo_content()
{
    auto& scene = project_.scene;

    const auto group = scene.create(ns::NodeType::Group, "Stage");
    if (auto* node = scene.find(group)) node->transform.position = {0.0f, 0.0f};

    const auto card = scene.create(ns::NodeType::Shape, "Card");
    scene.set_parent(card, group);
    if (auto* node = scene.find(card)) {
        node->transform.position = {120.0f, 150.0f};
        node->size = {180.0f, 110.0f};
        node->color = {0.95f, 0.55f, 0.35f, 1.0f};
    }

    const auto title = scene.create(ns::NodeType::Text, "Title");
    scene.set_parent(title, group);
    if (auto* node = scene.find(title)) {
        node->transform.position = {120.0f, 70.0f};
        node->text = "Hello cosine";
        node->text_size = 30.0f;
        node->color = {0.90f, 0.93f, 0.98f, 1.0f};
    }

    auto& track = project_.timeline.add_track(card, prop::Position);
    track.add_key({0.0f, glm::vec2{120.0f, 150.0f}, xel::anim::Easing::Linear});
    track.add_key({2.0f, glm::vec2{420.0f, 150.0f}, xel::anim::Easing::EaseInOut});
    project_.timeline.set_duration(2.0f);
    project_.timeline.set_loop(true);
}

void Editor::refresh_scene_tree()
{
    if (scene_tree_ == nullptr) return;

    std::function<ui::TreeNode(ns::NodeId)> build_node;
    build_node = [this, &build_node](ns::NodeId id) {
        ui::TreeNode node;
        const auto* source = project_.scene.find(id);
        if (source == nullptr) {
            node.title = "?";
            return node;
        }
        node.title = source->name.empty() ? ("node " + std::to_string(id)) : source->name;
        node.expanded = true;
        for (const ns::NodeId child : source->children) {
            node.children.push_back(build_node(child));
        }
        return node;
    };

    std::vector<ui::TreeNode> roots;
    const auto root_id = project_.scene.root();
    if (root_id != ns::kInvalidNode) roots.push_back(build_node(root_id));
    scene_tree_->set_roots(std::move(roots));
}

void Editor::refresh_property_panel()
{
    if (property_label_ != nullptr) property_label_->set_text("Properties (M6.2)");
}

void Editor::add_shape_node()
{
    ++shape_counter_;
    const auto id =
        project_.scene.create(ns::NodeType::Shape, "Shape " + std::to_string(shape_counter_));
    if (auto* node = project_.scene.find(id)) {
        node->size = {80.0f, 60.0f};
        node->transform.position = {150.0f + 30.0f * static_cast<float>(shape_counter_), 320.0f};
        node->color = {0.45f, 0.65f, 0.95f, 1.0f};
    }
    refresh_scene_tree();
    set_status("added node " + std::to_string(id));
}

void Editor::toggle_rounded_style()
{
    experimental_rounded_ = !experimental_rounded_;
    apply_theme();
    set_status(experimental_rounded_ ? "style: rounded (experimental)" : "style: classic");
}

void Editor::apply_theme()
{
    // classic 为默认外观：直角、无过渡、键盘焦点环。
    // 实验性圆角样式需要从 dark() 重新派生（with_rounding 是单向变换）。
    const ui::Theme theme = ui::Theme::dark()
                                .with_rounding(experimental_rounded_)
                                .with_radius_scale(experimental_rounded_ ? 1.0f : 0.0f)
                                .with_focus_policy(ui::FocusPolicy::Keyboard)
                                .with_transition_ms(experimental_rounded_ ? 120.0f : 0.0f);
    root_->set_theme(theme);
}

void Editor::layout()
{
    const float width = root_->width();
    const float height = root_->height();
    if (width <= 0.0f || height <= 0.0f) return;

    const float body_top = kMenuHeight + kToolbarHeight;
    const float status_y = height - kStatusHeight;
    const float available = std::max(240.0f, status_y - body_top);

    // 时间轴全宽横跨底部（2D 关键帧编辑需要横向空间）；
    // 属性面板只占工作区高度，不再贯穿到底。
    const float timeline_height = std::clamp(available * kTimelineRatio, 160.0f, 320.0f);
    const float work_height = std::max(120.0f, available - timeline_height);

    // 窄窗口下压缩侧栏，优先保证预览宽度。
    constexpr float kMinPreview = 200.0f;
    float tree_width = kTreeWidth;
    float property_width = kPropertyWidth;
    const float side_total = tree_width + property_width;
    if (width - side_total < kMinPreview) {
        const float shrink = std::max(0.0f, width - kMinPreview) / side_total;
        tree_width = std::max(140.0f, tree_width * shrink);
        property_width = std::max(200.0f, property_width * shrink);
    }
    const float preview_width = std::max(120.0f, width - tree_width - property_width);

    place(menu_, 0.0f, 0.0f, width, kMenuHeight);
    place(toolbar_, 0.0f, kMenuHeight, width, kToolbarHeight);
    place(scene_tree_, 0.0f, body_top, tree_width, work_height);
    place(preview_, tree_width, body_top, preview_width, work_height);
    place(property_panel_, tree_width + preview_width, body_top, property_width, work_height);
    place(timeline_panel_, 0.0f, body_top + work_height, width, timeline_height);
    place(status_, 0.0f, status_y, width, kStatusHeight);

    if (preview_ != nullptr) preview_->mark_layout_dirty();
}

void Editor::tick(float dt)
{
    time_ += dt;

    const float width = root_->width();
    const float height = root_->height();
    if (width != last_w_ || height != last_h_) {
        last_w_ = width;
        last_h_ = height;
        layout();
    }
}

void Editor::set_status(std::string text)
{
    if (status_ != nullptr) status_->set_text(std::move(text));
}
} // namespace xel::editor
