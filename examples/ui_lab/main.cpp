// 高级 UI 实验室（EX-2）：探索**未定 API** 的组合控件。
// 与 ui_gallery 的分工、纪律与结论表见同目录 README.md。
// 硬约束：产品代码不得依赖本目录；lab 不进 ctest 断言，只有冒烟门禁。

#include "ui/core/button.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/drag_float.hpp"
#include "ui/core/image.hpp"
#include "ui/views/file_browser.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/core/label.hpp"
#include "ui/containers/panel.hpp"
#include "ui/root.hpp"
#include "ui/core/slider.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/text_input.hpp"
#include "ui/theme.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/composites/section.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/core/tab_bar.hpp"
#include "ui/composites/window.hpp"
#include "graphics/text_debug.hpp"
#include "xel_application.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace
{
using xel::ui::Button;
using xel::ui::Checkbox;
using xel::ui::DragFloat;
using xel::ui::Label;
using xel::ui::Panel;
using xel::ui::Slider;
using xel::ui::TextInput;
using xel::ui::Vbox;

constexpr float kW = 1200.0f;
constexpr float kH = 800.0f;
constexpr float kColumnY = 62.0f;
constexpr float kColumnH = 690.0f;
constexpr float kLeftX = 16.0f;
constexpr float kLeftW = 300.0f;
constexpr float kRightX = 328.0f;
constexpr float kRightW = 856.0f;

std::string lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// ---------- 实验清单 ----------

struct Experiment
{
    const char* title;
    const char* hypothesis;
    bool implemented;
};

constexpr Experiment kExperiments[] = {
    {"E1 命令面板", "输入过滤 + 列表直选，比翻菜单快，值得做成常驻控件", true},
    {"E2 主题密度矩阵", "四个标量足以描述密度，不必引入紧凑/宽松预设", true},
    {"E3 时间轴非线性缩放", "局部放大需要 zoom + pan，固定像素每秒不够用", false},
    {"E4 多选批量编辑", "属性面板需要共同值 / 混合值显示与批量写入", false},
    {"E5 拖拽排序", "内联重排可复用长按 + 拖拽反馈，不必新事件", false},
    {"E6 内联重命名", "就地编辑比弹对话框顺手", false},
    {"E8 可拖动窗口", "imGui 式应用内窗口：拖动、置顶、resize、折叠、关闭", true},};
constexpr int kExperimentCount = static_cast<int>(std::size(kExperiments));

// ---------- E1 命令面板 ----------

struct Command
{
    const char* name;
    const char* hint;
};

constexpr Command kCommands[] = {
    {"Add Shape", "create a shape node"},
    {"Add Text", "create a text node"},
    {"Reset Time", "seek timeline to 0"},
    {"Step Frame", "advance one frame"},
    {"Save Project", "write .cos project"},
    {"Open Project", "read .cos project"},
    {"Export PNG", "render offscreen to png"},
    {"Toggle Theme", "dark / light"},
};

struct Palette
{
    TextInput* input = nullptr;
    Vbox* list = nullptr;
    Label* result = nullptr;
    int runs = 0;

    // 按输入内容重建列表：这里是实验的重点（过滤 + 直选的代价）。
    void rebuild()
    {
        if (list == nullptr) return;
        list->clear_children();

        const std::string query =
            input != nullptr ? lower(std::string{input->text()}) : std::string{};
        int shown = 0;
        for (const Command& command : kCommands) {
            const std::string name{command.name};
            if (!query.empty() && lower(name).find(query) == std::string::npos) continue;
            ++shown;
            list->add(std::make_unique<Button>(
                name + "    —    " + command.hint, [this, name] {
                    ++runs;
                    if (result != nullptr) {
                        result->set_text("ran \"" + name + "\"  ·  total runs " +
                                         std::to_string(runs));
                    }
                }));
        }
        if (shown == 0) list->add(std::make_unique<Label>("no matching command", 18.0f));
    }
};

// ---------- E2 密度矩阵 ----------

struct Density
{
    Slider* text_size = nullptr;
    Slider* control_height = nullptr;
    Slider* padding = nullptr;
    Slider* spacing = nullptr;
    Label* text_value = nullptr;
    Label* control_value = nullptr;
    Label* padding_value = nullptr;
    Label* spacing_value = nullptr;
};

// 本地控件名识别（参考 DebugOverlay 的判断链，lab 自用）。
std::string widget_name(const xel::ui::Widget* widget)
{
    if (widget == nullptr) return "-";
    if (dynamic_cast<const Label*>(widget)) return "Label";
    if (dynamic_cast<const Button*>(widget)) return "Button";
    if (dynamic_cast<const xel::ui::ColorRect*>(widget)) return "ColorRect";
    if (dynamic_cast<const xel::ui::TextInput*>(widget)) return "TextInput";
    if (dynamic_cast<const xel::ui::Slider*>(widget)) return "Slider";
    if (dynamic_cast<const xel::ui::Spacer*>(widget)) return "Spacer";
    if (dynamic_cast<const xel::ui::Image*>(widget)) return "Image";
    if (dynamic_cast<const xel::ui::FileBrowser*>(widget)) return "FileBrowser";
    if (dynamic_cast<const xel::ui::ScrollPanel*>(widget)) return "ScrollPanel";
    if (dynamic_cast<const xel::ui::Section*>(widget)) return "Section";
    if (dynamic_cast<const xel::ui::TabBar*>(widget)) return "TabBar";
    if (dynamic_cast<const xel::ui::Window*>(widget)) return "Window";
    if (dynamic_cast<const xel::ui::Panel*>(widget)) return "Panel";
    if (dynamic_cast<const xel::ui::Hbox*>(widget)) return "Hbox";
    if (dynamic_cast<const xel::ui::Vbox*>(widget)) return "Vbox";
    return "Widget";
}

void set_debug_recursive(xel::ui::Widget* widget, bool enabled)
{
    if (widget == nullptr) return;
    widget->set_debug(enabled);
    for (const auto& child : widget->children()) {
        set_debug_recursive(child.get(), enabled);
    }
}

bool widget_in_tree(const xel::ui::Widget* root, const xel::ui::Widget* target)
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

bool is_descendant(const xel::ui::Widget* widget, const xel::ui::Widget* ancestor)
{
    if (widget == nullptr || ancestor == nullptr) return false;
    for (const xel::ui::Widget* w = widget; w != nullptr; w = w->parent()) {
        if (w == ancestor) return true;
    }
    return false;
}

// ---------- 实验室外壳 ----------

struct Lab
{
    xel::ui::UIRoot* root = nullptr;
    Label* status = nullptr;
    Label* title = nullptr;
    Label* hypothesis = nullptr;
    Vbox* stage = nullptr;
    Palette palette;
    Density density;
    // E8 的窗口常驻挂在 root 上（不在 stage 里，否则被面板裁剪）。
    std::vector<xel::ui::Window*> windows;
    // 全局参数窗口（无标题栏、始终置顶）。
    xel::ui::Window* settings = nullptr;
    xel::ui::ScrollPanel* settings_scroll = nullptr;
    // Widget 调试页（EX-5）。
    xel::ui::Widget* widget_target = nullptr;
    xel::ui::Widget* widget_hovered = nullptr;
    xel::ui::Widget* widget_hover_candidate = nullptr;
    float widget_hover_time = 0.0f;
    bool widget_follow = true;
    Label* widget_target_label = nullptr;
    Label* widget_info_label = nullptr;
    Vbox* widget_children_box = nullptr;
    DragFloat* widget_pos_x = nullptr;
    DragFloat* widget_pos_y = nullptr;
    DragFloat* widget_size_w = nullptr;
    DragFloat* widget_size_h = nullptr;
    DragFloat* widget_z = nullptr;
    DragFloat* widget_flex = nullptr;
    Checkbox* widget_visible = nullptr;
    Checkbox* widget_absolute = nullptr;
    Checkbox* widget_debug = nullptr;
    Checkbox* widget_debug_children = nullptr;
    Checkbox* widget_follow_box = nullptr;
    bool widget_updating = false;
    bool widget_page_active = false;
    bool labels_debug = false;
    xel::ui::Theme theme;
    int current = -1;

    void set_status(std::string text)
    {
        if (status != nullptr) status->set_text(std::move(text));
    }

    void select(int index);
    void update_widget_debug(const xel::ui::Widget* hovered, const xel::ui::Widget* focused,
                             float dt);
    void update_widget_props();
    void clear_widget_page_refs();
    void rebuild_widget_children();
};

void apply_density(Lab& lab)
{
    const Density& d = lab.density;
    if (d.text_size == nullptr) return;

    xel::ui::Theme next = lab.theme;
    next.text_size = d.text_size->value();
    next.small_text_size = next.text_size * 0.8f;
    next.heading_text_size = next.text_size * 1.35f;
    if (d.control_height != nullptr) next.control_height = d.control_height->value();
    if (d.padding != nullptr) next.padding = d.padding->value();
    if (d.spacing != nullptr) next.spacing = d.spacing->value();
    lab.theme = next;

    if (lab.root != nullptr) lab.root->set_theme(lab.theme);
    lab.set_status("theme applied: text " + std::format("{:.0f}", next.text_size) + ", control " +
                   std::format("{:.0f}", next.control_height) + ", padding " +
                   std::format("{:.0f}", next.padding) + ", spacing " +
                   std::format("{:.0f}", next.spacing));
}

void Lab::select(int index)
{
    if (index < 0 || index >= kExperimentCount || index == current) return;
    current = index;
    // E8 的窗口常驻在 root 上，只在这个实验里显示。
    for (auto* window : windows) {
        if (window != nullptr) window->set_visible(index == 6);
    }

    if (stage == nullptr) return;
    // 全局 settings 面板在各个实验中保持可见。
    if (settings != nullptr) settings->set_visible(true);
    stage->clear_children();
    palette = Palette{};
    density = Density{};

    if (title != nullptr) title->set_text(kExperiments[index].title);
    if (hypothesis != nullptr) {
        hypothesis->set_text(std::string{"假设："} + kExperiments[index].hypothesis);
    }

    if (!kExperiments[index].implemented) {
        stage->add(std::make_unique<Label>("该实验尚未实现（见 README 的实验清单）", 18.0f));
        set_status(kExperiments[index].title);
        return;
    }

    if (index == 0) {
        stage->add(std::make_unique<Label>("输入过滤命令，点击执行：", 18.0f));

        auto input = std::make_unique<TextInput>("", 560.0f);
        palette.input = input.get();
        stage->add(std::move(input));

        auto list = std::make_unique<Vbox>(0.0f, 0.0f, 700.0f, 380.0f);
        list->set_spacing(4.0f);
        palette.list = list.get();
        stage->add(std::move(list));

        auto result = std::make_unique<Label>("尚未执行任何命令", 18.0f);
        palette.result = result.get();
        stage->add(std::move(result));

        palette.input->set_on_change([this](const std::string&) { palette.rebuild(); });
        palette.rebuild();
        set_status("E1 命令面板：输入以过滤，点击执行");
        return;
    }

    // E2：四个标量 + 应用按钮。
    // E8 可拖动窗口：窗口已在 build() 里挂到 root，这里只控制显隐。
    if (index == 6) {
        stage->add(std::make_unique<Label>(
            "窗口在整个 lab 区域可拖动（不受面板裁剪）：", 18.0f));
        stage->add(std::make_unique<Label>(
            "拖标题栏移动、拖右下角改大小、点左侧三角折叠、点右侧叉关闭", 18.0f));
        stage->add(std::make_unique<Label>(
            "三个窗口叠放在右侧，点哪个哪个置顶（z 归一化）", 18.0f));
        set_status("E8 可拖动窗口：三个叠放，验证置顶与 z 归一化");
        return;
    }

    stage->add(std::make_unique<Label>("拖动后点 Apply，观察既有布局如何反应：", 18.0f));

    auto add_row = [this](const char* name, float lo, float hi, float initial,
                          Slider** slider_out, Label** value_out) {
        auto row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 640.0f, 34.0f);
        row->set_spacing(10.0f);
        row->set_cross_align(xel::ui::Align::center_left());
        row->add(std::make_unique<Label>(name, 18.0f));

        auto slider = std::make_unique<Slider>(300.0f);
        slider->set_range(lo, hi);
        slider->set_value(initial);
        *slider_out = slider.get();

        auto value = std::make_unique<Label>(std::format("{:.0f}", initial), 18.0f);
        *value_out = value.get();

        Label* value_ptr = value.get();
        slider->set_on_change([value_ptr](float v) {
            value_ptr->set_text(std::format("{:.0f}", v));
        });

        row->add(std::move(slider));
        row->add(std::move(value));
        stage->add(std::move(row));
    };

    add_row("text size", 10.0f, 32.0f, theme.text_size, &density.text_size,
            &density.text_value);
    add_row("control height", 20.0f, 60.0f, theme.control_height, &density.control_height,
            &density.control_value);
    add_row("padding", 0.0f, 24.0f, theme.padding, &density.padding, &density.padding_value);
    add_row("spacing", 0.0f, 24.0f, theme.spacing, &density.spacing, &density.spacing_value);

    stage->add(std::make_unique<Button>("Apply to theme", [this] { apply_density(*this); }));
    set_status("E2 密度矩阵：调好后点 Apply");
}

// 字体 debug 是每个 Label 自己的选项，没有全局开关：遍历控件树统一设置。
void apply_label_debug(xel::ui::Widget* widget, const xel::graphics::TextDebugOptions& options)
{
    if (widget == nullptr) return;
    if (auto* label = dynamic_cast<Label*>(widget)) label->set_text_debug(options);
    else if (auto* button = dynamic_cast<Button*>(widget)) button->set_text_debug(options);
    else if (auto* window = dynamic_cast<xel::ui::Window*>(widget)) window->set_text_debug(options);
    for (const auto& child : widget->children()) apply_label_debug(child.get(), options);
    for (const auto& overlay : widget->overlays()) apply_label_debug(overlay.get(), options);
}

void build(Lab& lab, xel::ui::UIRoot& root)
{
    lab.root = &root;
    lab.theme = xel::ui::Theme::dark();
    // 最小字号 20：lab 是近距离阅读，字太小看不清。
    lab.theme.text_size = 20.0f;
    lab.theme.small_text_size = 20.0f;
    lab.theme.heading_text_size = 28.0f;
    lab.theme.control_height = 32.0f;
    root.set_theme(lab.theme);

    auto heading = std::make_unique<Label>("ui_lab · 高级 UI 实验室", 22.0f);
    heading->set_pos(16.0f, 10.0f);
    root.add(std::move(heading));

    auto note = std::make_unique<Label>("实验代码，产品不依赖；结论回填 examples/ui_lab/README.md",
                                        18.0f);
    note->set_pos(16.0f, 38.0f);
    root.add(std::move(note));

    auto left = std::make_unique<Panel>(kLeftX, kColumnY, kLeftW, kColumnH);
    left->set_cross_align(xel::ui::Align::h_stretch());
    left->add(std::make_unique<Label>("Experiments", 18.0f));
    for (int i = 0; i < kExperimentCount; ++i) {
        std::string title = kExperiments[i].title;
        if (!kExperiments[i].implemented) title += "  (todo)";
        left->add(std::make_unique<Button>(std::move(title), [&lab, i] { lab.select(i); }));
    }
    root.add(std::move(left));

    auto right = std::make_unique<Panel>(kRightX, kColumnY, kRightW, kColumnH);
    right->set_cross_align(xel::ui::Align::top_left());
    auto title = std::make_unique<Label>("选择一个实验", 18.0f);
    lab.title = title.get();
    right->add(std::move(title));

    auto hypothesis = std::make_unique<Label>("", 18.0f);
    lab.hypothesis = hypothesis.get();
    right->add(std::move(hypothesis));

    auto stage = std::make_unique<Vbox>(0.0f, 0.0f, kRightW - 24.0f, 500.0f);
    stage->set_spacing(6.0f);
    stage->set_cross_align(xel::ui::Align::top_left());
    lab.stage = stage.get();
    right->add(std::move(stage));
    root.add(std::move(right));

    auto status = std::make_unique<Label>("ready", 18.0f);
    status->set_pos(16.0f, kH - 26.0f);
    lab.status = status.get();
    root.add(std::move(status));

    // E8 可拖动窗口：挂在 root（全窗口区域、不被面板裁剪），切实验时只改可见性。
    const char* titles[] = {"Inspector", "Assets", "Timeline"};
    const glm::vec2 offsets[] = {{360.0f, 150.0f}, {420.0f, 210.0f}, {480.0f, 270.0f}};
    for (int i = 0; i < 3; ++i) {
        auto window = std::make_unique<xel::ui::Window>(titles[i], offsets[i],
                                                        glm::vec2{300.0f, 200.0f});
        window->add(std::make_unique<Label>(std::string{"window "} + std::to_string(i + 1), 18.0f));
        window->add(std::make_unique<Label>("drag the title bar", 18.0f));
        window->set_visible(false);
        lab.windows.push_back(window.get());
        root.add(std::move(window));
    }

    lab.select(0);

    // 全局参数窗口：无标题栏、始终置顶，拖窗口本体移动。
    // 分区结构：TabBar 选类目 → 状态行 → Section 折叠分组。
    auto settings = std::make_unique<xel::ui::Window>("settings", glm::vec2{760.0f, 12.0f},
                                                      glm::vec2{440.0f, 320.0f});
    settings->set_title_bar_visible(false);
    settings->set_always_on_top(true);
    settings->content()->set_cross_align(xel::ui::Align::h_stretch());

    auto tabs = std::make_unique<xel::ui::TabBar>();
    tabs->add_tab("Widget");
    tabs->add_tab("Font");
    tabs->add_tab("Theme");
    tabs->add_tab("Scene");
    tabs->set_size(426.0f, 36.0f);
    auto* tabs_ptr = tabs.get();
    settings->add(std::move(tabs));

    auto settings_status = std::make_unique<Label>("font debug: off", 18.0f);
    auto* settings_status_ptr = settings_status.get();
    settings->add(std::move(settings_status));

    // 直接使用 ScrollPanel 作为内容区：高度由 settings 窗口剩余空间决定，随 resize 跟随。
    auto scroll = std::make_unique<xel::ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    scroll->set_size_percent({1.0f, 0.0f});
    scroll->set_flex_grow(1.0f);
    scroll->set_min_size({0.0f, 0.0f});
    scroll->set_max_size({10000.0f, 10000.0f});
    auto* scroll_ptr = scroll.get();
    settings->add(std::move(scroll));
    lab.settings_scroll = scroll_ptr;

    // 内容切换时重建到 ScrollPanel 的内容区。
    auto fill_widget = [&lab, scroll_ptr, settings_status_ptr] {
        lab.clear_widget_page_refs();
        lab.widget_page_active = true;
        scroll_ptr->content()->clear_children();
        scroll_ptr->content()->add(std::make_unique<Label>("Widget · 当前控件", 20.0f));
        auto target = std::make_unique<Label>("target: -", 18.0f);
        lab.widget_target_label = target.get();
        scroll_ptr->content()->add(std::move(target));
        auto info = std::make_unique<Label>("hover a widget", 16.0f);
        lab.widget_info_label = info.get();
        scroll_ptr->content()->add(std::move(info));

        // 通用属性编辑行。
        auto pos_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        pos_row->set_padding(0.0f);
        pos_row->set_spacing(6.0f);
        pos_row->set_cross_align(xel::ui::Align::center_left());
        pos_row->add(std::make_unique<Label>("Pos", 16.0f));
        auto px = std::make_unique<DragFloat>("", 90.0f);
        px->set_range(-10000.0f, 10000.0f);
        lab.widget_pos_x = px.get();
        px->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            const auto b = lab.widget_target->local_bounds();
            lab.widget_target->set_pos(value, b.pos.y);
        });
        pos_row->add(std::move(px));
        auto py = std::make_unique<DragFloat>("", 90.0f);
        py->set_range(-10000.0f, 10000.0f);
        lab.widget_pos_y = py.get();
        py->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            const auto b = lab.widget_target->local_bounds();
            lab.widget_target->set_pos(b.pos.x, value);
        });
        pos_row->add(std::move(py));
        scroll_ptr->content()->add(std::move(pos_row));

        auto size_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        size_row->set_padding(0.0f);
        size_row->set_spacing(6.0f);
        size_row->set_cross_align(xel::ui::Align::center_left());
        size_row->add(std::make_unique<Label>("Size", 16.0f));
        auto sw = std::make_unique<DragFloat>("", 90.0f);
        sw->set_range(0.0f, 10000.0f);
        lab.widget_size_w = sw.get();
        sw->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            const auto b = lab.widget_target->local_bounds();
            lab.widget_target->set_size(value, b.size.y);
        });
        size_row->add(std::move(sw));
        auto sh = std::make_unique<DragFloat>("", 90.0f);
        sh->set_range(0.0f, 10000.0f);
        lab.widget_size_h = sh.get();
        sh->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            const auto b = lab.widget_target->local_bounds();
            lab.widget_target->set_size(b.size.x, value);
        });
        size_row->add(std::move(sh));
        scroll_ptr->content()->add(std::move(size_row));

        auto z_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        z_row->set_padding(0.0f);
        z_row->set_spacing(6.0f);
        z_row->set_cross_align(xel::ui::Align::center_left());
        z_row->add(std::make_unique<Label>("Z", 16.0f));
        auto z = std::make_unique<DragFloat>("", 90.0f);
        z->set_range(-1000.0f, 1000.0f);
        lab.widget_z = z.get();
        z->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            lab.widget_target->set_z_index(static_cast<int>(value + 0.5f));
        });
        z_row->add(std::move(z));
        scroll_ptr->content()->add(std::move(z_row));

        auto flex_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        flex_row->set_padding(0.0f);
        flex_row->set_spacing(6.0f);
        flex_row->set_cross_align(xel::ui::Align::center_left());
        flex_row->add(std::make_unique<Label>("Flex", 16.0f));
        auto flex = std::make_unique<DragFloat>("", 90.0f);
        flex->set_range(0.0f, 100.0f);
        lab.widget_flex = flex.get();
        flex->set_on_change([&lab](float value) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            lab.widget_target->set_flex_grow(value);
        });
        flex_row->add(std::move(flex));
        scroll_ptr->content()->add(std::move(flex_row));

        auto toggle_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        toggle_row->set_padding(0.0f);
        toggle_row->set_spacing(8.0f);
        toggle_row->set_cross_align(xel::ui::Align::center_left());
        auto visible = std::make_unique<Checkbox>("Visible", false, [&lab](bool checked) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            lab.widget_target->set_visible(checked);
        });
        lab.widget_visible = visible.get();
        toggle_row->add(std::move(visible));
        auto absolute = std::make_unique<Checkbox>("Absolute", false, [&lab](bool checked) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            lab.widget_target->set_absolute(checked);
        });
        lab.widget_absolute = absolute.get();
        toggle_row->add(std::move(absolute));
        scroll_ptr->content()->add(std::move(toggle_row));

        auto debug_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 36.0f);
        debug_row->set_padding(0.0f);
        debug_row->set_spacing(8.0f);
        debug_row->set_cross_align(xel::ui::Align::center_left());
        auto debug = std::make_unique<Checkbox>("Debug", false, [&lab](bool checked) {
            if (lab.widget_updating || lab.widget_target == nullptr) return;
            lab.widget_target->set_debug(checked);
            if (lab.root != nullptr) lab.root->set_debug(true);
        });
        lab.widget_debug = debug.get();
        debug_row->add(std::move(debug));
        auto debug_children = std::make_unique<Checkbox>("Debug children", false, [&lab](bool checked) {
            if (lab.widget_target == nullptr) return;
            set_debug_recursive(lab.widget_target, checked);
            if (lab.root != nullptr) lab.root->set_debug(true);
        });
        lab.widget_debug_children = debug_children.get();
        debug_row->add(std::move(debug_children));
        scroll_ptr->content()->add(std::move(debug_row));

        auto follow = std::make_unique<Checkbox>(
            "Follow hover", lab.widget_follow, [&lab](bool checked) {
                lab.widget_follow = checked;
            });
        lab.widget_follow_box = follow.get();
        scroll_ptr->content()->add(std::move(follow));
        scroll_ptr->content()->add(std::make_unique<Button>("Clear", [&lab] {
            lab.widget_target = nullptr;
            lab.rebuild_widget_children();
            lab.update_widget_props();
        }));
        auto children = std::make_unique<Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        children->set_spacing(4.0f);
        children->set_cross_align(xel::ui::Align::top_left());
        lab.widget_children_box = children.get();
        scroll_ptr->content()->add(std::move(children));
        lab.rebuild_widget_children();
        (void)settings_status_ptr;
        (void)settings_status_ptr;
    };

    auto fill_font = [&lab, scroll_ptr, settings_status_ptr] {
        lab.clear_widget_page_refs();
        lab.widget_page_active = false;
        scroll_ptr->content()->clear_children();
        scroll_ptr->content()->add(std::make_unique<Label>("Font · 文字与调试绘制", 20.0f));
        scroll_ptr->content()->add(std::make_unique<Button>("Toggle font debug", [&lab, settings_status_ptr] {
            lab.labels_debug = !lab.labels_debug;
            xel::graphics::TextDebugOptions options;
            options.glyph_boxes = lab.labels_debug;
            options.line_box = lab.labels_debug;
            options.baseline = lab.labels_debug;
            apply_label_debug(lab.root, options);
            if (settings_status_ptr != nullptr) {
                settings_status_ptr->set_text(lab.labels_debug ? "font debug: on" : "font debug: off");
            }
            lab.set_status(lab.labels_debug ? "font debug: on" : "font debug: off");
        }));
        scroll_ptr->content()->add(std::make_unique<Button>("Glyph boxes only", [&lab] {
            xel::graphics::TextDebugOptions options;
            options.glyph_boxes = true;
            apply_label_debug(lab.root, options);
            lab.set_status("font debug: glyph boxes");
        }));
        scroll_ptr->content()->add(std::make_unique<Label>(
            "调试绘制作用于现有控件，新建控件需再点一次", 18.0f));
    };

    auto fill_theme = [&lab, scroll_ptr] {
        lab.clear_widget_page_refs();
        lab.widget_page_active = false;
        scroll_ptr->content()->clear_children();
        scroll_ptr->content()->add(std::make_unique<Label>("Theme · 主题与密度", 20.0f));
        scroll_ptr->content()->add(std::make_unique<Label>(
            "text " + std::format("{:.0f}", lab.theme.text_size) + "    control " +
                std::format("{:.0f}", lab.theme.control_height) + "    padding " +
                std::format("{:.0f}", lab.theme.padding) + "    spacing " +
                std::format("{:.0f}", lab.theme.spacing),
            18.0f));
        scroll_ptr->content()->add(
            std::make_unique<Label>("密度实时调参：见 E2 主题密度矩阵", 18.0f));
        scroll_ptr->content()->add(
            std::make_unique<Label>("令牌色板编辑：待 UI-3 后续", 18.0f));
    };

    auto fill_scene = [&lab, scroll_ptr] {
        lab.clear_widget_page_refs();
        lab.widget_page_active = false;
        scroll_ptr->content()->clear_children();
        scroll_ptr->content()->add(std::make_unique<Label>("Scene · 场景与导出", 20.0f));
        scroll_ptr->content()->add(std::make_unique<Label>(
            std::string{"current: "} + kExperiments[lab.current < 0 ? 0 : lab.current].title,
            18.0f));
        scroll_ptr->content()->add(std::make_unique<Button>("Next experiment", [&lab] {
            lab.select((lab.current + 1) % kExperimentCount);
        }));
        scroll_ptr->content()->add(
            std::make_unique<Label>("离屏导出：见 E1 的 Export PNG", 18.0f));
    };

    std::vector<std::function<void()>> fills{fill_widget, fill_font, fill_theme, fill_scene};
    tabs_ptr->set_on_change([fills, &lab](int index) {
        if (index >= 0 && index < static_cast<int>(fills.size())) {
            fills[static_cast<std::size_t>(index)]();
        }
        // 内容重建后立刻重排：只标 dirty 要等下一帧，且父容器持有旧的测量缓存。
        if (lab.root != nullptr) {
            lab.root->invalidate_measure();
            lab.root->layout();
        }
    });
    tabs_ptr->set_selected(0, false);
    fill_widget();

    // 导出验证用：直接切到指定页，走一遍「重建 + 重排」的路径。
    if (const char* tab = std::getenv("COSINE_LAB_SETTINGS_TAB")) {
        tabs_ptr->set_selected(std::atoi(tab), true);
    }

    lab.settings = settings.get();
    root.add(std::move(settings));

    // 离屏导出时用环境变量直接开启，省得手动点按钮。
    if (const char* debug = std::getenv("COSINE_LAB_FONT_DEBUG");
        debug != nullptr && debug[0] == '1') {
        lab.labels_debug = true;
        xel::graphics::TextDebugOptions options;
        options.glyph_boxes = true;
        options.line_box = true;
        options.baseline = true;
        apply_label_debug(lab.root, options);
        lab.set_status("font debug: on");
    }
    if (const char* env = std::getenv("COSINE_LAB_EXPERIMENT")) {
        lab.select(std::atoi(env) - 1);
    }
}

void Lab::clear_widget_page_refs()
{
    widget_target_label = nullptr;
    widget_info_label = nullptr;
    widget_children_box = nullptr;
    widget_pos_x = nullptr;
    widget_pos_y = nullptr;
    widget_size_w = nullptr;
    widget_size_h = nullptr;
    widget_z = nullptr;
    widget_flex = nullptr;
    widget_visible = nullptr;
    widget_absolute = nullptr;
    widget_debug = nullptr;
    widget_debug_children = nullptr;
    widget_follow_box = nullptr;
}

void Lab::rebuild_widget_children()
{
    if (widget_children_box == nullptr) return;
    widget_children_box->clear_children();
    if (widget_target == nullptr) return;

    if (auto* parent = widget_target->parent()) {
        widget_children_box->add(std::make_unique<Button>(
            "^ parent: " + widget_name(parent), [this, parent] {
                widget_target = parent;
                widget_follow = false;
                if (widget_follow_box != nullptr) widget_follow_box->set_checked(false);
                rebuild_widget_children();
            }));
    }

    widget_children_box->add(std::make_unique<Label>("children:", 16.0f));
    for (const auto& child : widget_target->children()) {
        auto* ptr = child.get();
        widget_children_box->add(std::make_unique<Button>(
            widget_name(ptr), [this, ptr] {
                widget_target = ptr;
                widget_follow = false;
                if (widget_follow_box != nullptr) widget_follow_box->set_checked(false);
                rebuild_widget_children();
            }));
    }
}

void Lab::update_widget_debug(const xel::ui::Widget* hovered, const xel::ui::Widget* focused,
                              float dt)
{
    (void)focused;
    if (!widget_page_active) return;
    // 目标失效（被销毁/移出 root/进入调试窗口）时清空。
    if (widget_target != nullptr
        && (!widget_in_tree(root, widget_target) || is_descendant(widget_target, settings))) {
        widget_target = nullptr;
        widget_hover_candidate = nullptr;
        widget_hover_time = 0.0f;
        rebuild_widget_children();
    }

    // 抓取：鼠标在可调试控件上“停留”一小段时间才成为目标。
    // 这样从目标移向 settings 面板时，途经的控件不会被误抓；移到面板或空白后保持原目标。
    widget_hovered = nullptr;
    if (hovered != nullptr && hovered != root && !is_descendant(hovered, settings)) {
        widget_hovered = const_cast<xel::ui::Widget*>(hovered);
        if (widget_follow) {
            if (widget_hovered == widget_hover_candidate) {
                widget_hover_time += dt;
            } else {
                widget_hover_candidate = widget_hovered;
                widget_hover_time = 0.0f;
            }
            if (widget_hover_time >= 0.12f && widget_target != widget_hovered) {
                widget_target = widget_hovered;
                rebuild_widget_children();
            }
        }
    } else {
        widget_hover_candidate = nullptr;
        widget_hover_time = 0.0f;
    }

    if (widget_target_label != nullptr) {
        widget_target_label->set_text("target: " + widget_name(widget_target));
    }
    if (widget_info_label != nullptr) {
        if (widget_target == nullptr) {
            widget_info_label->set_text("hover a widget");
        } else {
            const auto& bounds = widget_target->global_bounds();
            widget_info_label->set_text(std::format(
                "pos {:.0f},{:.0f}  size {:.0f}x{:.0f}  visible {}  z {}  children {}",
                bounds.pos.x, bounds.pos.y, bounds.size.x, bounds.size.y,
                widget_target->visible() ? "yes" : "no", widget_target->z_index(),
                widget_target->children().size()));
        }
    }
    update_widget_props();
}

void Lab::update_widget_props()
{
    widget_updating = true;
    const xel::ui::Widget* target = widget_target;
    const xel::ui::Rect bounds = target ? target->local_bounds() : xel::ui::Rect{};
    if (widget_pos_x != nullptr) widget_pos_x->set_value(bounds.pos.x);
    if (widget_pos_y != nullptr) widget_pos_y->set_value(bounds.pos.y);
    if (widget_size_w != nullptr) widget_size_w->set_value(bounds.size.x);
    if (widget_size_h != nullptr) widget_size_h->set_value(bounds.size.y);
    if (widget_z != nullptr) {
        widget_z->set_value(target != nullptr ? static_cast<float>(target->z_index()) : 0.0f);
    }
    if (widget_flex != nullptr) {
        widget_flex->set_value(target != nullptr ? target->flex_grow() : 0.0f);
    }
    if (widget_visible != nullptr) {
        widget_visible->set_checked(target != nullptr && target->visible());
    }
    if (widget_absolute != nullptr) {
        widget_absolute->set_checked(target != nullptr && target->absolute());
    }
    if (widget_debug != nullptr) {
        widget_debug->set_checked(target != nullptr && target->is_debug());
    }
    if (widget_debug_children != nullptr) {
        widget_debug_children->set_checked(target != nullptr && target->is_debug());
    }
    widget_updating = false;
}

int run()
{
    xel::Application app{static_cast<int>(kW), static_cast<int>(kH), "ui_lab"};

    Lab lab;
    build(lab, *app.uisys.root());

    // 离屏导出：与其它示例一致的固定尺寸，便于人工核对布局。
    if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
        app.uisys.tick(1.0f / 60.0f);
        lab.update_widget_debug(app.uisys.hovered(), app.uisys.focused(), 1.0f / 60.0f);

        auto* root = app.uisys.root();
        root->set_size(kW, kH);
        root->set_global_bounds({{0.0f, 0.0f}, {kW, kH}});
        root->mark_layout_dirty();
        root->layout();
        root->clear_layout_dirty();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        const auto pixels = app.renderer_.render_offscreen(*root, static_cast<uint32_t>(kW),
                                                          static_cast<uint32_t>(kH));
        const bool ok = xel::backend::vulkan::Renderer::write_png(
            png, static_cast<uint32_t>(kW), static_cast<uint32_t>(kH), pixels);
        std::cout << (ok ? "exported " : "export failed ") << png << '\n';
        return ok ? 0 : 1;
    }

    const char* exit_after_env = std::getenv("COSINE_EXIT_AFTER");
    const float exit_after = exit_after_env != nullptr ? std::atof(exit_after_env) : 0.0f;
    float total_time = 0.0f;

    while (!app.window_.should_close()) {
        glfwPollEvents();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        app.renderer_.begin();

        const auto& stats = app.renderer_.frame_stats();
        const float dt = std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);
        app.uisys.tick(dt);
        lab.update_widget_debug(app.uisys.hovered(), app.uisys.focused(), dt);

        app.renderer_.draw(*app.uisys.root());        app.renderer_.end();

        total_time += dt;
        if (exit_after > 0.0f && total_time >= exit_after) {
            glfwSetWindowShouldClose(app.window_.get_glfw_window(), GLFW_TRUE);
        }
    }
    return 0;
}
} // namespace

int main()
{
    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
