#pragma once

#include "ui/composites/scroll_panel.hpp"
#include "ui/composites/tab_view.hpp"
#include "ui/composites/window.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/root.hpp"

#include <memory>
#include <string>
#include <vector>

namespace xel::editor
{
class Editor;

// 编辑器设置窗口：左侧竖向标签栏（TabView + TabBar Vertical），右侧页面内嵌 ScrollPanel。
class SettingsWindow
{
public:
    SettingsWindow(ui::UIRoot& root, Editor& editor);
    ~SettingsWindow();

    void show();
    void hide();
    bool visible() const;

private:
    void build();
    void select_group(int index);

    void build_general(ui::Vbox& group);
    void build_preview(ui::Vbox& group);
    void build_export(ui::Vbox& group);
    void build_about(ui::Vbox& group);

    Editor* editor_ = nullptr;
    ui::Window* window_ = nullptr;
    ui::TabView* tab_view_ = nullptr;
    std::vector<ui::Vbox*> groups_;
    int current_ = 0;
};
} // namespace xel::editor
