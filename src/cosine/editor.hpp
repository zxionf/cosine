#pragma once

#include "anim/project.hpp"
#include "ui/label.hpp"
#include "ui/menu_bar.hpp"
#include "ui/panel.hpp"
#include "ui/root.hpp"
#include "ui/scene_view.hpp"
#include "ui/tree_view.hpp"
#include "ui/ui_system.hpp"
#include "ui/widget.hpp"

#include <cstdint>
#include <memory>
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
    Editor(ui::UIRoot& root, ui::UISystem& uisys);

    // 每帧调用：跟踪窗口尺寸变化并重排。
    void tick(float dt);

    anim::Project& project() { return project_; }
    const anim::Project& project() const { return project_; }
    bool experimental_rounded() const { return experimental_rounded_; }

private:
    void build();
    void build_menu();
    void build_toolbar();
    void build_panels();
    void build_demo_content();

    void refresh_scene_tree();
    void refresh_property_panel();
    void add_shape_node();
    void toggle_rounded_style();

    void apply_theme();
    void layout();
    void set_status(std::string text);

    ui::UIRoot* root_ = nullptr;
    ui::UISystem* uisys_ = nullptr;
    anim::Project project_;

    ui::MenuBar* menu_ = nullptr;
    ui::Widget* toolbar_ = nullptr;
    ui::TreeView* scene_tree_ = nullptr;
    ui::SceneView* preview_ = nullptr;
    ui::Panel* property_panel_ = nullptr;
    ui::Panel* timeline_panel_ = nullptr;
    ui::Label* property_label_ = nullptr;
    ui::Label* timeline_label_ = nullptr;
    ui::Label* status_ = nullptr;

    bool experimental_rounded_ = false;
    float last_w_ = 0.0f;
    float last_h_ = 0.0f;
    float time_ = 0.0f;
    int shape_counter_ = 0;
};
} // namespace xel::editor
