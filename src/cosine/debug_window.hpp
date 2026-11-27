#pragma once

#include "ui/composites/scroll_panel.hpp"
#include "ui/composites/tab_view.hpp"
#include "ui/composites/window.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/drag_float.hpp"
#include "ui/core/label.hpp"
#include "ui/system/root.hpp"
#include "ui/system/ui_system.hpp"
#include "ui/node/widget.hpp"

#include <memory>

namespace xel::editor
{
// 运行时调试窗口：
//   Widget 页：悬停抓取控件、查看/编辑通用属性、开关 debug 绘制；
//   Font 页：文字调试绘制（glyph boxes / line box / baseline）。
class DebugWindow
{
public:
    explicit DebugWindow(ui::UIRoot& root);
    ~DebugWindow();

    void show();
    void hide();
    bool visible() const;

    // 在 uisys 处理完输入后调用；dt 用于 hover 停留计时。
    void update(const ui::UISystem& uisys, float dt);

private:
    void build();
    void build_widget_page(ui::ScrollPanel& page);
    void build_font_page(ui::ScrollPanel& page);
    void build_layout_page(ui::ScrollPanel& page);
    void rebuild_children();
    void update_props();
    void update_layout_page();
    void apply_text_debug(bool enabled, bool glyph_only);
    bool is_own(const ui::Widget* widget) const;

    ui::UIRoot* root_ = nullptr;
    ui::Window* window_ = nullptr;

    ui::Widget* target_ = nullptr;
    ui::Widget* hovered_ = nullptr;
    ui::Widget* hover_candidate_ = nullptr;
    float hover_time_ = 0.0f;
    bool follow_ = true;
    bool updating_ = false;
    bool font_debug_ = false;

    ui::Label* target_label_ = nullptr;
    ui::Label* info_label_ = nullptr;
    ui::Vbox* children_box_ = nullptr;
    ui::DragFloat* pos_x_ = nullptr;
    ui::DragFloat* pos_y_ = nullptr;
    ui::DragFloat* size_w_ = nullptr;
    ui::DragFloat* size_h_ = nullptr;
    ui::DragFloat* z_ = nullptr;
    ui::DragFloat* flex_ = nullptr;
    ui::Checkbox* visible_ = nullptr;
    ui::Checkbox* absolute_ = nullptr;
    ui::Checkbox* debug_ = nullptr;
    ui::Checkbox* debug_children_ = nullptr;
    ui::Checkbox* follow_box_ = nullptr;

    // Layout 页（UI-36）：只读展示目标控件的布局状态（不提供编辑）。
    ui::Label* layout_bounds_ = nullptr;
    ui::Label* layout_size_ = nullptr;
    ui::Label* layout_state_ = nullptr;
    ui::Label* layout_spec_ = nullptr;
    ui::Label* layout_content_ = nullptr;
};
} // namespace xel::editor
