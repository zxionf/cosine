#pragma once

#include "anim/command.hpp"
#include "anim/player.hpp"
#include "anim/project.hpp"
#include "backend/vulkan/renderer.hpp"
#include "texture_cache.hpp"
#include "ui/button.hpp"
#include "ui/color_input.hpp"
#include "ui/dialog.hpp"
#include "ui/drag_float.hpp"
#include "ui/label.hpp"
#include "ui/menu_bar.hpp"
#include "ui/panel.hpp"
#include "ui/preview_ruler.hpp"
#include "ui/root.hpp"
#include "ui/scene_view.hpp"
#include "ui/splitter.hpp"
#include "ui/timeline_view.hpp"
#include "ui/tree_view.hpp"
#include "ui/ui_system.hpp"
#include "ui/widget.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace xel::editor
{
// 编辑器外壳（M6.4a 骨架）。
// 职责：持有 Project、组装菜单/工具栏/三栏面板、应用主题、按窗口尺寸排版。
// 面板内容（属性编辑、时间轴）在 M6.2 / M6.3 填入，Editor 只负责挂载与转发。
//
// 默认外观：classic（直角、无过渡）。带圆角的现代样式是实验性选项，
// 由「视图 → Experimental rounded style」切换。
// 滚动默认模式沿用控件默认值（Smooth），此处不再覆盖。
class Editor
{
public:
    Editor(ui::UIRoot& root, ui::UISystem& uisys, backend::vulkan::Renderer& renderer);

    // 每帧调用：跟踪窗口尺寸变化并重排。
    void tick(float dt);

    anim::Project& project() { return project_; }
    const anim::Project& project() const { return project_; }
    bool experimental_rounded() const { return experimental_rounded_; }

private:
    void build();
    void build_backdrop();
    void toggle_playback();
    void undo();
    void redo();
    void build_menu();
    void build_toolbar();
    void build_panels();
    std::unique_ptr<ui::Widget> build_property_panel();
    void build_demo_content();

    void refresh_scene_tree();
    void on_scene_selected(const std::vector<int>& path);
    std::optional<scene::NodeId> node_from_path(const std::vector<int>& path) const;
    void sync_property_panel();
    void add_shape_node();
    void step_frame(int direction);
    void toggle_rounded_style();

    // 项目文件（M6.5）。
    void new_project();
    void open_project_dialog();
    void load_project(const std::string& path);
    void save_project();
    void save_project_as_dialog();
    void write_project(const std::string& path);

    // 图片导入（CO-2）：上传到图片槽并绑定到选中节点（无选中时用 demo 节点）。
    bool import_image(const std::string& path);
    // 把所有带 texture_path 的 Image 节点重新绑定到纹理槽（重载项目后调用）。
    void rebind_textures();

    void apply_theme();
    void layout();
    void set_status(std::string text);

    ui::UIRoot* root_ = nullptr;
    ui::UISystem* uisys_ = nullptr;
    anim::Project project_;
    std::string project_path_;
    std::unique_ptr<anim::Player> player_;
    ui::DialogHost* file_dialog_ = nullptr;
    backend::vulkan::Renderer* renderer_ = nullptr;
    TextureCache texture_cache_;
    anim::CommandStack stack_{256};
    float pending_before_ = 0.0f;

    ui::MenuBar* menu_ = nullptr;
    ui::Panel* menu_bg_ = nullptr;
    ui::Panel* toolbar_bg_ = nullptr;
    ui::Widget* toolbar_ = nullptr;
    ui::Button* play_button_ = nullptr;

    // 可拖分区：上下（工作区 / 时间轴）、左右（树 / 右区）、左右（预览 / 属性）。
    // 嵌套顺序保证「拖属性条」只改预览与属性的宽度，树宽不变。
    ui::Splitter* root_splitter_ = nullptr;
    ui::Splitter* outer_splitter_ = nullptr;
    ui::Splitter* right_splitter_ = nullptr;
    ui::TreeView* scene_tree_ = nullptr;
    ui::SceneView* preview_ = nullptr;
    ui::PreviewRuler* preview_ruler_ = nullptr;
    ui::Panel* property_panel_ = nullptr;
    ui::TimelineView* timeline_view_ = nullptr;
    ui::Label* property_label_ = nullptr;
    ui::Label* status_ = nullptr;

    // 属性面板行（M6.2）。
    ui::DragFloat* pos_x_ = nullptr;
    ui::DragFloat* pos_y_ = nullptr;
    ui::DragFloat* rotation_ = nullptr;
    ui::DragFloat* scale_x_ = nullptr;
    ui::DragFloat* scale_y_ = nullptr;
    ui::DragFloat* size_w_ = nullptr;
    ui::DragFloat* size_h_ = nullptr;
    ui::ColorInput* color_ = nullptr;
    ui::DragFloat* color_a_ = nullptr;

    scene::NodeId selected_ = scene::kInvalidNode;
    scene::NodeId demo_card_ = scene::kInvalidNode;

    bool experimental_rounded_ = false;
    float last_w_ = 0.0f;
    float last_h_ = 0.0f;
    float time_ = 0.0f;
    int shape_counter_ = 0;
};
} // namespace xel::editor
