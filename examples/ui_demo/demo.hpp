#pragma once

// ── XEL UI Demo：控件与布局的展示台 ────────────────────────────────
//
// 设计目标：
//   1. 广度：核心控件、容器、数据视图、浮层、媒体各自有分区展示；
//   2. 深度：主题 / 密度 / 焦点策略 / DPI / 过渡 / 语义 / 性能指标都能现场改并看到结果；
//   3. 可扩展：新增内容 = 在注册表里加一条 Section（id/title/summary/build），
//      build 只依赖 Demo 与一列 Vbox，不需要碰外壳代码。
//
// 外壳结构（本文件负责）：
//   ┌ 第一行：MenuBar + 主题/密度/缩放开关 ───────────────────────────┐
//   ├ 第二行：TabView 标签页导航（每个分区一个标签）│ 内容区（可滚动）   │
//   └ 状态栏：分区名 · fps/frame/draw · 焦点/悬停 · 最近一次交互 ─────┘
//
// 页面内不使用可折叠分组：分组只是「标题 + 内容」的卡片，避免与「页签切换」
// 产生两种并存的收起/展开语义。

#include "graphics/frame_stats.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/composites/tab_view.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/label.hpp"
#include "ui/core/typography.hpp"
#include "ui/node/widget.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/root.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace demo
{
// 本 demo 全部代码都在 namespace demo 内，用别名避免每处写 xel::。
namespace ui = xel::ui;
namespace graphics = xel::graphics;

class Demo;

// 一个分区 = 注册表里的一条记录。build 只在首次进入该标签页时调用一次。
using BuildFn = void (*)(Demo&, ui::Vbox& column);

struct Section
{
    std::string id;      // 唯一标识；COSINE_DEMO_SECTION=<id> 可直达
    std::string title;   // 标签页标题
    std::string summary; // 页首的一句话说明
    BuildFn build = nullptr;
};

// 分区注册表：想加内容，只改这里（实现见 demo.cpp）。
const std::vector<Section>& section_list();

// 外壳里需要被外部读写的部分（main 每帧刷新状态栏）。
struct Shell
{
    ui::TabView* tabs = nullptr;  // 第二行的标签页导航 + 页面区
    ui::Label* status = nullptr;  // 底部状态栏文本
};

// 演示台状态：主题、DPI、分区切换、交互反馈。
class Demo
{
public:
    // set_ui_scale 由 main 注入（Application::set_ui_scale）；留空则不提供缩放开关。
    Demo(ui::UIRoot& root, std::function<void(float)> set_ui_scale);
    ~Demo();

    Demo(const Demo&) = delete;
    Demo& operator=(const Demo&) = delete;

    ui::UIRoot& root() { return *root_; }
    Shell& shell() { return shell_; }
    const Section& section() const;

    // ── 分区 ──────────────────────────────────────────────────────
    void show_section(int index);
    void show_section(const std::string& id);
    int current_index() const { return current_; }
    // 已构建内容的页面数（懒构建：没打开过的标签页不算）。
    int built_page_count() const;

    // ── 主题（广度：明暗 / 圆角 / 焦点策略 / 过渡 / 密度）──────────────
    void toggle_dark();
    void set_dark(bool dark);
    bool dark() const { return dark_; }
    void toggle_rounded();
    void cycle_focus_policy();
    void set_density(ui::Density density);
    void cycle_density();
    void toggle_transitions();

    // ── DPI ──────────────────────────────────────────────────────
    void set_ui_scale(float scale);
    float ui_scale() const { return ui_scale_; }

    // ── 交互反馈：任何分区都可以调用，写到状态栏 ──────────────────────
    void note(std::string text);

    // ── 每帧（main 调用）：刷新状态栏 ──────────────────────────────
    void update(float dt, const graphics::FrameStats& stats, ui::Widget* hovered,
                ui::Widget* focused);

private:
    void build_shell();
    void build_top_bar(ui::Vbox& column);
    void build_status_bar(ui::Vbox& column);
    // 首次进入某分区时把内容建进对应的页面列。
    void ensure_page(int index);
    void apply_theme();

    ui::UIRoot* root_ = nullptr;
    Shell shell_;
    std::function<void(float)> set_ui_scale_;

    bool dark_ = true;
    bool rounded_ = true;
    ui::FocusPolicy focus_policy_ = ui::FocusPolicy::Keyboard;
    ui::Density density_ = ui::Density::Normal;
    bool transitions_ = true;
    float ui_scale_ = 1.0f;

    int current_ = -1;
    std::vector<ui::Vbox*> page_columns_;  // 每个标签页的内容列（懒填充）
    std::vector<bool> page_built_;

    std::string note_ = "就绪：点第二行的标签页切换分区，或用顶栏菜单";
};

// 创建演示台：构建外壳 + 显示首个分区（或 COSINE_DEMO_SECTION 指定的分区）。
std::unique_ptr<Demo> create(ui::UIRoot& root, std::function<void(float)> set_ui_scale);

// ── 分区构建辅助（各 sections/*.cpp 共用）─────────────────────────
// 说明文字（自动换行）。
ui::Paragraph& hint(ui::Widget& parent, std::string text);
// 水平行：交叉轴居中、横向铺满；返回可继续 add 的 Hbox。
ui::Hbox& row(ui::Widget& parent, float spacing = 8.0f);
// 分组卡片：标题行 + 内容列，**不可折叠**（返回内容列，直接往里 add）。
ui::Vbox& group(ui::Vbox& column, std::string title);
// 间距块。
void gap(ui::Widget& parent, float height);
} // namespace demo
