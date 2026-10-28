#include "editor.hpp"

#include "preview_pane.hpp"

// Scene 类型位于 `xel::scene`（不是 `xel::anim::scene`）。
// 这里在本 TU 内提供 `anim::scene` 别名，避免编辑器代码逐处写长名字。
namespace xel::anim { namespace scene = ::xel::scene; }

#include "anim/easing.hpp"
#include "anim/property.hpp"
#include "anim/serialize.hpp"
#include "ui/button.hpp"
#include "ui/file_browser.hpp"
#include "ui/hbox.hpp"
#include "ui/scroll_panel.hpp"
#include "ui/text_input.hpp"
#include "ui/theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <functional>
#include <string>
#include <utility>

namespace xel::editor
{
namespace
{
namespace ns = xel::scene;
namespace prop = xel::anim::prop;

constexpr float kMenuHeight = 44.0f;
constexpr float kToolbarHeight = 62.0f;
constexpr float kTreeWidth = 280.0f;
constexpr float kPropertyWidth = 380.0f;
constexpr float kTimelineRatio = 0.28f;
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
constexpr float kRadToDeg = 180.0f / 3.14159265358979323846f;
constexpr float kStatusHeight = 28.0f;

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

Editor::Editor(ui::UIRoot& root, ui::UISystem& uisys, backend::vulkan::Renderer& renderer)
    : root_(&root), uisys_(&uisys), renderer_(&renderer)
{
    // 验证用：COSINE_EDITOR_ROUNDED=1 启动即启用实验性圆角样式。
    if (std::getenv("COSINE_EDITOR_ROUNDED") != nullptr) experimental_rounded_ = true;

    build_demo_content();
    // Player 只持引用，Project 先于它存在且生命周期更长。
    player_ = std::make_unique<anim::Player>(project_.scene, project_.timeline);
    player_->play();
    build();
    apply_theme();
    layout();

    // 验证用：COSINE_EDITOR_OPEN=1 启动即打开项目对话框（便于离屏截图检查布局）。
    if (std::getenv("COSINE_EDITOR_OPEN") != nullptr) open_project_dialog();

    // 验证用：COSINE_EDITOR_IMAGE=<png> 启动时导入一张图（CO-2 第一段的自测入口）。
    if (const char* image = std::getenv("COSINE_EDITOR_IMAGE")) import_image(image);
}

void Editor::build()
{
    build_backdrop();
    build_menu();
    build_toolbar();
    build_panels();
}

void Editor::toggle_playback()
{
    if (player_ == nullptr) return;
    player_->toggle();
    if (play_button_ != nullptr) {
        play_button_->set_label(player_->playing() ? "Pause" : "Play");
    }
    set_status(player_->playing() ? "playing" : "paused");
}

void Editor::step_frame(int direction)
{
    if (player_ == nullptr) return;
    constexpr float kFrameStep = 1.0f / 30.0f;
    const float duration = std::max(project_.timeline.duration(), kFrameStep);

    // 逐帧先暂停，否则下一轮 tick 又会用播放时间覆盖。
    player_->pause();
    if (play_button_ != nullptr) play_button_->set_label("Play");

    const float target = std::clamp(time_ + static_cast<float>(direction) * kFrameStep, 0.0f,
                                    duration);
    player_->seek(target);
    time_ = target;
    if (timeline_view_ != nullptr) timeline_view_->set_time(target);
    if (preview_ruler_ != nullptr) preview_ruler_->set_time(target);
    set_status(std::format("frame {:.2f}s", target));
}

void Editor::undo()
{
    if (!stack_.undo(project_)) {
        set_status("nothing to undo");
        return;
    }
    sync_property_panel();
    if (preview_ != nullptr) preview_->mark_layout_dirty();
    set_status("undo");
}

void Editor::redo()
{
    if (!stack_.redo(project_)) {
        set_status("nothing to redo");
        return;
    }
    sync_property_panel();
    if (preview_ != nullptr) preview_->mark_layout_dirty();
    set_status("redo");
}

void Editor::build_backdrop()
{
    // 菜单栗 / 工具栏的底色条：先 add，保证绘制在内容之下。
    auto menu_bg = std::make_unique<ui::Panel>(0.0f, 0.0f, 1280.0f, kMenuHeight);
    menu_bg_ = menu_bg.get();
    root_->add(std::move(menu_bg));

    auto toolbar_bg = std::make_unique<ui::Panel>(0.0f, 0.0f, 1280.0f, kToolbarHeight);
    toolbar_bg_ = toolbar_bg.get();
    root_->add(std::move(toolbar_bg));
}

void Editor::build_menu()
{
    auto menu = std::make_unique<ui::MenuBar>();
    menu_ = menu.get();

    menu->add_menu("File", {
        ui::MenuItem::make("New", [this] { new_project(); }, "Ctrl+N"),
        ui::MenuItem::make("Open...", [this] { open_project_dialog(); }, "Ctrl+O"),
        ui::MenuItem::make("Save", [this] { save_project(); }, "Ctrl+S"),
        ui::MenuItem::make("Save As...", [this] { save_project_as_dialog(); }),
        ui::MenuItem::separator_item(),
        ui::MenuItem::make("Exit", [this] { set_status("close the window to exit"); }, "Ctrl+Q"),
    });

    menu->add_menu("Edit", {
        ui::MenuItem::make("Undo", [this] { undo(); }, "Ctrl+Z"),
        ui::MenuItem::make("Redo", [this] { redo(); }, "Ctrl+Y"),
    });

    menu->add_menu("View", {
        ui::MenuItem::make("Experimental rounded style",
                           [this] { toggle_rounded_style(); }),
    });

    menu->add_menu("Playback", {
        ui::MenuItem::make("Play / Pause", [this] { toggle_playback(); }, "Space"),
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
    toolbar->set_cross_align(ui::Align::center_left()); // 垂直居中，避免按钮顶到条边
    toolbar_ = toolbar.get();

    auto add = [&toolbar](std::string label, std::function<void()> action) {
        toolbar->add(std::make_unique<ui::Button>(std::move(label), std::move(action)));
    };
    auto play = std::make_unique<ui::Button>("Pause", [this] { toggle_playback(); });
    play_button_ = play.get();
    toolbar->add(std::move(play));

    add("◀ Frame", [this] { step_frame(-1); });
    add("Frame ▶", [this] { step_frame(1); });
    add("Reset", [this] {
        if (player_ == nullptr) return;
        player_->seek(0.0f);
        time_ = 0.0f;
        if (timeline_view_ != nullptr) timeline_view_->set_time(0.0f);
        set_status("reset to 0s");
    });
    add("Add Shape", [this] { add_shape_node(); });
    add("Style", [this] { toggle_rounded_style(); });

    root_->add(std::move(toolbar));
}

void Editor::build_panels()
{
    auto tree = std::make_unique<ui::TreeView>(kTreeWidth, 300.0f);
    scene_tree_ = tree.get();
    tree->set_row_height(38.0f); // 字号放大后行高要跟上，否则文字被裁
    tree->set_min_size({160.0f, 160.0f});
    tree->set_max_size({1.0e6f, 1.0e6f});
    tree->set_on_select([this](const std::vector<int>& path) { on_scene_selected(path); });

    auto preview = std::make_unique<ui::SceneView>(&project_.scene);
    preview_ = preview.get();
    preview->set_min_size({200.0f, 150.0f});

    // 预览底部的时间标尺：显示进度，点击 / 拖动即 seek。
    auto ruler = std::make_unique<ui::PreviewRuler>();
    preview_ruler_ = ruler.get();
    ruler->set_duration(project_.timeline.duration());
    ruler->set_on_seek([this](float seconds) {
        if (player_ != nullptr) player_->seek(seconds);
        time_ = seconds;
        if (timeline_view_ != nullptr) timeline_view_->set_time(seconds);
    });
    auto preview_pane = std::make_unique<PreviewPane>(std::move(preview), std::move(ruler), 28.0f);

    auto property = build_property_panel();

    // 时间轴：标尺 + 轨道 + 关键帧（M6.3）。
    auto timeline = std::make_unique<ui::TimelineView>(&project_.timeline);
    timeline_view_ = timeline.get();
    timeline->set_pixels_per_second(120.0f);
    timeline->set_on_seek([this](float seconds) {
        if (player_ != nullptr) player_->seek(seconds);
        time_ = seconds;
        set_status(std::format("seek {:.2f}s", seconds));
    });
    timeline->set_on_key_moved([this] {
        if (preview_ != nullptr) preview_->mark_layout_dirty();
        set_status("keyframe moved");
    });
    timeline->set_on_changed([this] {
        if (preview_ != nullptr) preview_->mark_layout_dirty();
        set_status("timeline edited");
    });

    // 时间轴编辑上报 → 命令栈（可撤销）。多选拖动合成一条复合命令。
    timeline->set_on_move_end([this](const std::vector<ui::KeyMove>& moves) {
        const auto& tracks = project_.timeline.tracks();
        std::vector<std::unique_ptr<anim::Command>> children;
        for (const ui::KeyMove& move : moves) {
            if (move.track < 0 || move.track >= static_cast<int>(tracks.size())) continue;
            const auto& entry = tracks[static_cast<std::size_t>(move.track)];
            children.push_back(std::make_unique<anim::MoveKeyCommand>(
                entry.node(), entry.property(), move.from, move.to));
        }
        if (children.empty()) return;
        if (children.size() == 1) {
            stack_.push(std::move(children.front()), project_);
        } else {
            stack_.push(std::make_unique<anim::CompoundCommand>(std::move(children)), project_);
        }
        set_status("keyframe moved");
    });
    timeline->set_on_remove_key([this](int track, float time) {
        const auto& tracks = project_.timeline.tracks();
        if (track < 0 || track >= static_cast<int>(tracks.size())) return;
        const auto& entry = tracks[static_cast<std::size_t>(track)];
        stack_.push(
            std::make_unique<anim::RemoveKeyCommand>(entry.node(), entry.property(), time), project_);
        set_status("keyframe removed");
    });
    timeline->set_on_cycle_easing(
        [this](int track, float time, anim::Easing from, anim::Easing to) {
            const auto& tracks = project_.timeline.tracks();
            if (track < 0 || track >= static_cast<int>(tracks.size())) return;
            const auto& entry = tracks[static_cast<std::size_t>(track)];
            stack_.push(std::make_unique<anim::SetKeyEasingCommand>(
                            entry.node(), entry.property(), time, from, to),
                        project_);
            set_status("easing changed");
        });
    timeline->set_min_size({400.0f, 120.0f});

    // 嵌套三份 Splitter：预览|属性 → 树|右区 → 工作区|时间轴。
    // 这样拖「属性」分割条只影响预览与属性，树宽不变。
    auto right = std::make_unique<ui::Splitter>(0.0f, 0.0f, ui::Splitter::Orientation::Horizontal);
    right->set_fixed_second(kPropertyWidth); // 属性固定宽，预览弹性
    right->set_panes(std::move(preview_pane), std::move(property));
    right_splitter_ = right.get();

    auto outer = std::make_unique<ui::Splitter>(0.0f, 0.0f, ui::Splitter::Orientation::Horizontal);
    outer->set_fixed_first(kTreeWidth); // 树固定宽，右区弹性
    outer->set_panes(std::move(tree), std::move(right));
    outer_splitter_ = outer.get();

    auto root_split =
        std::make_unique<ui::Splitter>(0.0f, 0.0f, ui::Splitter::Orientation::Vertical);
    root_split->set_ratio(1.0f - kTimelineRatio);
    root_split->set_panes(std::move(outer), std::move(timeline));
    // 首帧 root 尺寸可能还是 0（Editor::layout 会 early return）：
    // 给一个非零下限，避免 clamp_size 得到 0 尺寸后 ScrollView 轨道长度为 0。
    root_split->set_min_size({400.0f, 300.0f});
    root_splitter_ = root_split.get();
    root_->add(std::move(root_split));

    auto status = std::make_unique<ui::Label>("ready", 18.0f);
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

std::unique_ptr<ui::Widget> Editor::build_property_panel()
{
    // 属性行数多于可用高度，用可滚动面板。
    auto panel = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, kPropertyWidth, 400.0f);
    property_panel_ = panel.get();

    auto title = std::make_unique<ui::Label>("Properties — no selection", 20.0f);
    property_label_ = title.get();
    panel->add(std::move(title));

    auto add = [this, &panel](ui::DragFloat*& slot, std::string label, float lo, float hi,
                              int precision, std::string suffix,
                              std::function<float(const anim::scene::Node&)> read,
                              std::function<void(anim::scene::Node&, float)> write) {
        const std::string row_label = label;
        auto drag = std::make_unique<ui::DragFloat>(std::move(label), kPropertyWidth - 36.0f);
        drag->set_range(lo, hi);
        drag->set_precision(precision);
        drag->set_suffix(std::move(suffix));
        drag->set_enabled(false);

        // 拖动开始记 before，结束后把整次拖动合成一条命令。
        drag->set_on_drag_begin([this, read] {
            if (selected_ == anim::scene::kInvalidNode) return;
            if (const auto* node = project_.scene.find(selected_)) pending_before_ = read(*node);
        });
        drag->set_on_change([this, write](float value) {
            if (selected_ == anim::scene::kInvalidNode) return;
            if (auto* target = project_.scene.find(selected_)) {
                write(*target, value);
                if (preview_ != nullptr) preview_->mark_layout_dirty();
            }
        });

        slot = drag.get();
        ui::DragFloat* self = drag.get();
        drag->set_on_drag_end([this, write, self, row_label] {
            if (selected_ == anim::scene::kInvalidNode) return;
            stack_.push(std::make_unique<anim::SetNodeFieldCommand>(
                            selected_, row_label, write, pending_before_, self->value()),
                        project_);
            set_status("edited " + row_label);
        });
        // 键盘微调：每次按键一条命令。
        drag->set_on_commit([this, write, row_label](float before, float after) {
            if (selected_ == anim::scene::kInvalidNode) return;
            stack_.push(std::make_unique<anim::SetNodeFieldCommand>(
                            selected_, row_label, write, before, after),
                        project_);
            set_status("edited " + row_label);
        });

        panel->add(std::move(drag));
    };

    constexpr float kLimit = 4000.0f;
    add(pos_x_, "Pos X", -kLimit, kLimit, 1, "",
        [](const anim::scene::Node& n) { return n.transform.position.x; },
        [](anim::scene::Node& n, float v) { n.transform.position.x = v; });
    add(pos_y_, "Pos Y", -kLimit, kLimit, 1, "",
        [](const anim::scene::Node& n) { return n.transform.position.y; },
        [](anim::scene::Node& n, float v) { n.transform.position.y = v; });
    add(rotation_, "Rotation", -360.0f, 360.0f, 1, "deg",
        [](const anim::scene::Node& n) { return n.transform.rotation * kRadToDeg; },
        [](anim::scene::Node& n, float v) { n.transform.rotation = v * kDegToRad; });
    add(scale_x_, "Scale X", 0.01f, 100.0f, 2, "",
        [](const anim::scene::Node& n) { return n.transform.scale.x; },
        [](anim::scene::Node& n, float v) { n.transform.scale.x = v; });
    add(scale_y_, "Scale Y", 0.01f, 100.0f, 2, "",
        [](const anim::scene::Node& n) { return n.transform.scale.y; },
        [](anim::scene::Node& n, float v) { n.transform.scale.y = v; });
    add(size_w_, "Size W", 0.0f, kLimit, 1, "",
        [](const anim::scene::Node& n) { return n.size.x; },
        [](anim::scene::Node& n, float v) { n.size.x = v; });
    add(size_h_, "Size H", 0.0f, kLimit, 1, "",
        [](const anim::scene::Node& n) { return n.size.y; },
        [](anim::scene::Node& n, float v) { n.size.y = v; });

    // 颜色：一行预设色板（保留原 alpha）+ 独立的 alpha 行。
    auto color = std::make_unique<ui::ColorInput>("Color", kPropertyWidth - 36.0f);
    color_ = color.get();
    color->set_enabled(false);
    color->set_on_change([this](glm::vec4 value) {
        if (selected_ == anim::scene::kInvalidNode) return;
        if (auto* target = project_.scene.find(selected_)) {
            target->color = glm::vec4{value.r, value.g, value.b, target->color.a};
            if (preview_ != nullptr) preview_->mark_layout_dirty();
        }
    });
    color->set_on_commit([this](glm::vec4 before, glm::vec4 after) {
        if (selected_ == anim::scene::kInvalidNode) return;
        if (const auto* node = project_.scene.find(selected_)) {
            // 色板不改 alpha，保留节点原值（before.a 即当前 alpha）。
            before.a = node->color.a;
            after.a = node->color.a;
        }
        stack_.push(std::make_unique<anim::SetNodeColorCommand>(selected_, before, after), project_);
        set_status("edited Color");
    });
    panel->add(std::move(color));

    add(color_a_, "Color A", 0.0f, 1.0f, 2, "",
        [](const anim::scene::Node& n) { return n.color.a; },
        [](anim::scene::Node& n, float v) { n.color.a = v; });

    // 交给 Splitter 分配宽高，只限制下限。
    panel->set_min_size({240.0f, 200.0f});
    return panel;
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
    if (timeline_view_ != nullptr) timeline_view_->set_selected_node(selected_);
    sync_property_panel();
    set_status(selected_ == anim::scene::kInvalidNode ? "scene: no selection"
                                                      : "scene: node " + std::to_string(selected_));
}

void Editor::sync_property_panel()
{
    const anim::scene::Node* node =
        selected_ == anim::scene::kInvalidNode ? nullptr : project_.scene.find(selected_);

    ui::DragFloat* const rows[] = {pos_x_,  pos_y_, rotation_, scale_x_, scale_y_,
                                   size_w_, size_h_, color_a_};
    if (node == nullptr) {
        for (ui::DragFloat* drag : rows) {
            if (drag != nullptr) drag->set_enabled(false);
        }
        if (color_ != nullptr) color_->set_enabled(false);
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
    if (color_ != nullptr) {
        color_->set_enabled(true);
        color_->set_value(node->color);
    }
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

void Editor::new_project()
{
    project_ = anim::Project{};
    stack_.clear();
    project_path_.clear();
    selected_ = scene::kInvalidNode;
    demo_card_ = scene::kInvalidNode;
    time_ = 0.0f;

    if (player_ != nullptr) {
        player_->seek(0.0f);
        player_->play();
    }
    if (timeline_view_ != nullptr) timeline_view_->set_time(0.0f);
    if (preview_ruler_ != nullptr) preview_ruler_->set_duration(project_.timeline.duration());
    refresh_scene_tree();
    if (scene_tree_ != nullptr) scene_tree_->set_selected_path({});
    sync_property_panel();
    if (preview_ != nullptr) preview_->mark_layout_dirty();
    set_status("new project");
}

void Editor::open_project_dialog()
{
    if (root_ == nullptr || file_dialog_ != nullptr) return;

    auto dialog = std::make_unique<ui::Dialog>("Open project", 640.0f);
    ui::Dialog* dialog_ptr = dialog.get();

    auto browser = std::make_unique<ui::FileBrowser>(glm::vec2{600.0f, 260.0f});
    ui::FileBrowser* browser_ptr = browser.get();
    std::error_code dir_ec;
    const std::filesystem::path cwd = std::filesystem::current_path(dir_ec);
    browser->set_directory(dir_ec ? std::string{"."} : cwd.string());
    browser->set_extension_filter(".cos");

    auto name = std::make_unique<ui::TextInput>("", 400.0f);
    ui::TextInput* name_ptr = name.get();

    browser->set_on_select([browser_ptr, name_ptr] {
        const std::string& picked = browser_ptr->selected_name();
        if (picked.empty() || picked == "..") return;
        name_ptr->set_text(picked);
    });
    browser->set_on_activate([this, dialog_ptr, browser_ptr, name_ptr] {
        const std::string picked = browser_ptr->selected_name();
        if (picked.empty()) return;
        name_ptr->set_text(picked);
        const std::string path = browser_ptr->resolve(picked);
        dialog_ptr->dismiss();
        load_project(path);
    });

    dialog->add(std::move(browser));

    auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 600.0f, 40.0f);
    row->set_spacing(8.0f);
    row->set_cross_align(ui::Align::center_left());
    row->add(std::make_unique<ui::Label>("File", 18.0f));
    row->add(std::move(name));
    dialog->add(std::move(row));

    dialog->add_action("Open", [this, dialog_ptr, browser_ptr, name_ptr] {
        const std::string path = browser_ptr->resolve(std::string{name_ptr->text()});
        dialog_ptr->dismiss();
        if (path.empty()) {
            set_status("open: no file selected");
            return;
        }
        load_project(path);
    }, true);
    dialog->add_action("Cancel", [dialog_ptr] { dialog_ptr->dismiss(); });

    file_dialog_ = ui::show_dialog(*root_, std::move(dialog), [this] { file_dialog_ = nullptr; });
}

void Editor::load_project(const std::string& path)
{
    anim::Project loaded;
    std::string error;
    if (!anim::load_project_file(path, loaded, &error)) {
        set_status("open failed: " + error);
        return;
    }

    project_ = std::move(loaded);
    project_path_ = path;
    stack_.clear();
    selected_ = scene::kInvalidNode;
    demo_card_ = scene::kInvalidNode;
    time_ = 0.0f;

    if (player_ != nullptr) player_->seek(0.0f);
    if (timeline_view_ != nullptr) timeline_view_->set_time(0.0f);
    if (preview_ruler_ != nullptr) preview_ruler_->set_duration(project_.timeline.duration());
    rebind_textures();
    refresh_scene_tree();
    if (scene_tree_ != nullptr) scene_tree_->set_selected_path({});
    sync_property_panel();
    if (preview_ != nullptr) preview_->mark_layout_dirty();
    set_status("opened " + path);
}

void Editor::save_project()
{
    if (project_path_.empty()) {
        save_project_as_dialog();
        return;
    }
    write_project(project_path_);
}

void Editor::write_project(const std::string& path)
{
    std::string error;
    if (!anim::save_project_file(path, project_, &error)) {
        set_status("save failed: " + error);
        return;
    }
    project_path_ = path;
    set_status("saved " + path);
}

bool Editor::import_image(const std::string& path)
{
    if (renderer_ == nullptr || path.empty()) {
        set_status("import failed: no renderer or empty path");
        return false;
    }

    const std::uint32_t handle = texture_cache_.load(path, *renderer_);
    if (handle == 0) {
        set_status("import failed: " + path);
        return false;
    }

    // 没有选中时退回 demo 卡片，这样 COSINE_EDITOR_IMAGE 能直接看到效果。
    scene::NodeId target = selected_;
    if (target == scene::kInvalidNode) target = demo_card_;
    auto* node = project_.scene.find(target);
    if (node == nullptr) {
        set_status("imported " + path + " (no node to bind)");
        return false;
    }

    node->type = scene::NodeType::Image;
    node->texture_path = path;
    node->texture_id = handle;
    // 图片默认不叠色，否则默认 color 会把画面压暗。
    node->color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};

    if (preview_ != nullptr) preview_->mark_layout_dirty();
    refresh_scene_tree();
    sync_property_panel();
    set_status("imported " + path);
    return true;
}

void Editor::rebind_textures()
{
    if (renderer_ == nullptr) return;

    // 旧槽可能属于上一个项目，先清映射再按新项目的路径重新上传。
    texture_cache_.clear();
    for (const auto& [id, source] : project_.scene.nodes()) {
        if (source.type != scene::NodeType::Image || source.texture_path.empty()) continue;
        const std::uint32_t handle = texture_cache_.load(source.texture_path, *renderer_);
        if (auto* target = project_.scene.find(id)) target->texture_id = handle;
    }

    if (preview_ != nullptr) preview_->mark_layout_dirty();
}

void Editor::save_project_as_dialog()
{
    if (root_ == nullptr || file_dialog_ != nullptr) return;

    auto dialog = std::make_unique<ui::Dialog>("Save project", 640.0f);
    ui::Dialog* dialog_ptr = dialog.get();

    auto browser = std::make_unique<ui::FileBrowser>(glm::vec2{600.0f, 240.0f});
    ui::FileBrowser* browser_ptr = browser.get();
    const std::string start_dir = project_path_.empty()
        ? std::filesystem::current_path().string()
        : std::filesystem::path(project_path_).parent_path().string();
    browser->set_directory(start_dir);
    browser->set_extension_filter(".cos");

    const std::string initial_name = project_path_.empty()
        ? std::string{"untitled.cos"}
        : std::filesystem::path(project_path_).filename().string();
    auto name = std::make_unique<ui::TextInput>(initial_name, 400.0f);
    ui::TextInput* name_ptr = name.get();

    browser->set_on_select([browser_ptr, name_ptr] {
        const std::string& picked = browser_ptr->selected_name();
        if (picked.empty() || picked == "..") return;
        name_ptr->set_text(picked);
    });

    dialog->add(std::move(browser));

    auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 600.0f, 40.0f);
    row->set_spacing(8.0f);
    row->set_cross_align(ui::Align::center_left());
    row->add(std::make_unique<ui::Label>("Name", 18.0f));
    row->add(std::move(name));
    dialog->add(std::move(row));

    dialog->add_action("Save", [this, dialog_ptr, browser_ptr, name_ptr] {
        std::string name_text{name_ptr->text()};
        if (name_text.empty()) {
            set_status("save: empty file name");
            return;
        }
        if (std::filesystem::path(name_text).extension().empty()) name_text += ".cos";
        const std::string path = browser_ptr->resolve(name_text);
        dialog_ptr->dismiss();
        write_project(path);
    }, true);
    dialog->add_action("Cancel", [dialog_ptr] { dialog_ptr->dismiss(); });

    file_dialog_ = ui::show_dialog(*root_, std::move(dialog), [this] { file_dialog_ = nullptr; });
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
    const ui::Theme base = ui::Theme::dark()
                               .with_rounding(experimental_rounded_)
                               .with_radius_scale(experimental_rounded_ ? 1.0f : 0.0f)
                               .with_focus_policy(ui::FocusPolicy::Always)
                               .with_transition_ms(experimental_rounded_ ? 120.0f : 0.0f);
    // 编辑器比 gallery 观看距离更远：字号与控件高度适度放大。
    ui::Theme theme = base;
    theme.text_size = 24.0f;
    theme.small_text_size = 18.0f;
    theme.heading_text_size = 32.0f;
    theme.control_height = 44.0f;
    theme.padding = 10.0f;
    theme.spacing = 8.0f;
    root_->set_theme(theme);
}

void Editor::layout()
{
    const float width = root_->width();
    const float height = root_->height();
    if (width <= 0.0f || height <= 0.0f) return;

    const float body_top = kMenuHeight + kToolbarHeight;
    const float status_y = height - kStatusHeight;

    place(menu_bg_, 0.0f, 0.0f, width, kMenuHeight);
    place(menu_, 0.0f, 0.0f, width, kMenuHeight);
    place(toolbar_bg_, 0.0f, kMenuHeight, width, kToolbarHeight);
    place(toolbar_, 0.0f, kMenuHeight, width, kToolbarHeight);
    // 工作区与时间轴由 Splitter 分配，这里只给分割器整体定位。
    place(root_splitter_, 0.0f, body_top, width, std::max(240.0f, status_y - body_top));
    place(status_, 0.0f, status_y, width, kStatusHeight);

    if (preview_ != nullptr) preview_->mark_layout_dirty();
}

void Editor::tick(float dt)
{
    if (player_ != nullptr) {
        player_->tick(dt);
        time_ = player_->time();
        if (timeline_view_ != nullptr) timeline_view_->set_time(time_);
        if (preview_ruler_ != nullptr) {
            const float duration = project_.timeline.duration();
            if (preview_ruler_->duration() != duration) preview_ruler_->set_duration(duration);
            preview_ruler_->set_time(time_);
        }
    }

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
