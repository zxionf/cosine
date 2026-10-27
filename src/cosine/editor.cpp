#include "editor.hpp"

// Scene 类型位于 `xel::scene`（不是 `xel::anim::scene`）。
// 这里在本 TU 内提供 `anim::scene` 别名，避免编辑器代码逐处写长名字。
namespace xel::anim { namespace scene = ::xel::scene; }

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
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
constexpr float kRadToDeg = 180.0f / 3.14159265358979323846f;
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
    tree->set_on_select([this](const std::vector<int>& path) { on_scene_selected(path); });
    root_->add(std::move(tree));

    auto preview = std::make_unique<ui::SceneView>(&project_.scene);
    preview_ = preview.get();
    root_->add(std::move(preview));

    build_property_panel();

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

    // 默认选中 Card（root → Stage → Card = {0, 0, 0}），属性面板一打开就有内容。
    if (demo_card_ != ns::kInvalidNode && scene_tree_ != nullptr) {
        scene_tree_->set_selected_path({0, 0, 0});
    }
}

void Editor::build_demo_content()
{
    auto& scene = project_.scene;

    const auto group = scene.create(ns::NodeType::Group, "Stage");
    if (auto* node = scene.find(group)) node->transform.position = {0.0f, 0.0f};

    const auto card = scene.create(ns::NodeType::Shape, "Card");
    scene.set_parent(card, group);
    demo_card_ = card;
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

void Editor::build_property_panel()
{
    auto panel = std::make_unique<ui::Panel>(0.0f, 0.0f, kPropertyWidth, 400.0f);
    property_panel_ = panel.get();

    auto title = std::make_unique<ui::Label>("Properties — no selection", 13.0f);
    property_label_ = title.get();
    panel->add(std::move(title));

    auto add = [this, &panel](ui::DragFloat*& slot, std::string label, float lo, float hi,
                              int precision, std::string suffix,
                              std::function<void(anim::scene::Node&, float)> apply) {
        auto drag = std::make_unique<ui::DragFloat>(std::move(label), kPropertyWidth - 36.0f);
        drag->set_range(lo, hi);
        drag->set_precision(precision);
        drag->set_suffix(std::move(suffix));
        drag->set_enabled(false);
        drag->set_on_change([this, apply = std::move(apply)](float value) {
            if (selected_ == anim::scene::kInvalidNode) return;
            if (auto* target = project_.scene.find(selected_)) {
                apply(*target, value);
                if (preview_ != nullptr) preview_->mark_layout_dirty();
            }
        });
        slot = drag.get();
        panel->add(std::move(drag));
    };

    constexpr float kLimit = 4000.0f;
    add(pos_x_, "Pos X", -kLimit, kLimit, 1, "",
        [](anim::scene::Node& n, float v) { n.transform.position.x = v; });
    add(pos_y_, "Pos Y", -kLimit, kLimit, 1, "",
        [](anim::scene::Node& n, float v) { n.transform.position.y = v; });
    add(rotation_, "Rotation", -360.0f, 360.0f, 1, "deg",
        [](anim::scene::Node& n, float v) { n.transform.rotation = v * kDegToRad; });
    add(scale_x_, "Scale X", 0.01f, 100.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.transform.scale.x = v; });
    add(scale_y_, "Scale Y", 0.01f, 100.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.transform.scale.y = v; });
    add(size_w_, "Size W", 0.0f, kLimit, 1, "",
        [](anim::scene::Node& n, float v) { n.size.x = v; });
    add(size_h_, "Size H", 0.0f, kLimit, 1, "",
        [](anim::scene::Node& n, float v) { n.size.y = v; });
    add(color_r_, "Color R", 0.0f, 1.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.color.r = v; });
    add(color_g_, "Color G", 0.0f, 1.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.color.g = v; });
    add(color_b_, "Color B", 0.0f, 1.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.color.b = v; });
    add(color_a_, "Color A", 0.0f, 1.0f, 2, "",
        [](anim::scene::Node& n, float v) { n.color.a = v; });

    root_->add(std::move(panel));
}

std::optional<anim::scene::NodeId> Editor::node_from_path(const std::vector<int>& path) const
{
    if (path.empty()) return std::nullopt;

    // roots 只有一个（场景 root），path[0] 恒为 0；从 path[1] 开始逐层下钻。
    anim::scene::NodeId id = project_.scene.root();
    if (id == anim::scene::kInvalidNode) return std::nullopt;

    for (std::size_t i = 1; i < path.size(); ++i) {
        const anim::scene::Node* node = project_.scene.find(id);
        if (node == nullptr) return std::nullopt;
        const auto index = static_cast<std::size_t>(path[i]);
        if (index >= node->children.size()) return std::nullopt;
        id = node->children[index];
    }
    return id;
}

void Editor::on_scene_selected(const std::vector<int>& path)
{
    selected_ = node_from_path(path).value_or(anim::scene::kInvalidNode);
    sync_property_panel();
    set_status(selected_ == anim::scene::kInvalidNode ? "scene: no selection"
                                                      : "scene: node " + std::to_string(selected_));
}

void Editor::sync_property_panel()
{
    const anim::scene::Node* node =
        selected_ == anim::scene::kInvalidNode ? nullptr : project_.scene.find(selected_);

    ui::DragFloat* const rows[] = {pos_x_,   pos_y_,   rotation_, scale_x_, scale_y_, size_w_,
                                   size_h_,  color_r_, color_g_,  color_b_, color_a_};
    if (node == nullptr) {
        for (ui::DragFloat* drag : rows) {
            if (drag != nullptr) drag->set_enabled(false);
        }
        if (property_label_ != nullptr) property_label_->set_text("Properties — no selection");
        return;
    }

    if (property_label_ != nullptr) {
        const std::string name =
            node->name.empty() ? ("node " + std::to_string(node->id)) : node->name;
        property_label_->set_text("Properties — " + name);
    }

    auto set = [](ui::DragFloat* drag, float value) {
        if (drag == nullptr) return;
        drag->set_enabled(true);
        drag->set_value(value);
    };
    set(pos_x_, node->transform.position.x);
    set(pos_y_, node->transform.position.y);
    set(rotation_, node->transform.rotation * kRadToDeg);
    set(scale_x_, node->transform.scale.x);
    set(scale_y_, node->transform.scale.y);
    set(size_w_, node->size.x);
    set(size_h_, node->size.y);
    set(color_r_, node->color.r);
    set(color_g_, node->color.g);
    set(color_b_, node->color.b);
    set(color_a_, node->color.a);
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
                                .with_focus_policy(ui::FocusPolicy::Always)
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
