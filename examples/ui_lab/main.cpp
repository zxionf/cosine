// 高级 UI 实验室（EX-2）：探索**未定 API** 的组合控件。
// 与 ui_gallery 的分工、纪律与结论表见同目录 README.md。
// 硬约束：产品代码不得依赖本目录；lab 不进 ctest 断言，只有冒烟门禁。

#include "ui/button.hpp"
#include "ui/file_browser.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/root.hpp"
#include "ui/slider.hpp"
#include "ui/text_input.hpp"
#include "ui/theme.hpp"
#include "ui/vbox.hpp"
#include "ui/section.hpp"
#include "ui/tab_bar.hpp"
#include "ui/window.hpp"
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
    {"E7 资源浏览器", "从 cosine 的打开对话框搬来，再按「资源」用途打磨", true},
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

// ---------- E7 资源浏览器 ----------

constexpr const char* kResourceFilters[] = {"All", "Images", "Fonts", "Projects"};
constexpr const char* kResourceExtensions[] = {"", ".png", ".ttf", ".cos"};

std::string format_bytes(std::uintmax_t bytes)
{
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        return std::format("{:.1f} KB", static_cast<double>(bytes) / 1024.0);
    }
    return std::format("{:.1f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
}

struct Resource
{
    xel::ui::FileBrowser* list = nullptr;
    xel::ui::Hbox* crumbs = nullptr;
    Label* info_name = nullptr;
    Label* info_size = nullptr;
    Label* info_extra = nullptr;
    int filter = 0;
    std::function<void()> rebuild_crumbs;
    std::function<void()> refresh_info;
};

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
    Resource resource;
    // E8 的窗口常驻挂在 root 上（不在 stage 里，否则被面板裁剪）。
    std::vector<xel::ui::Window*> windows;
    // 全局参数窗口（无标题栏、始终置顶）。
    xel::ui::Window* settings = nullptr;
    bool labels_debug = false;
    xel::ui::Theme theme;
    int current = -1;

    void set_status(std::string text)
    {
        if (status != nullptr) status->set_text(std::move(text));
    }

    void select(int index);
};

void build_resource(Lab& lab, Vbox* stage)
{
    Resource& res = lab.resource;
    res = Resource{};

    // 面包屑：Up + 逐级路径按钮（最多 6 级，避免撑爆宽度）。
    auto crumb_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 820.0f, 30.0f);
    crumb_row->set_spacing(4.0f);
    crumb_row->set_cross_align(xel::ui::Align::center_left());
    res.crumbs = crumb_row.get();
    stage->add(std::move(crumb_row));

    // 类型过滤。
    auto filter_row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 820.0f, 32.0f);
    filter_row->set_spacing(6.0f);
    filter_row->set_cross_align(xel::ui::Align::center_left());
    filter_row->add(std::make_unique<Label>("Filter:", 18.0f));
    for (int i = 0; i < 4; ++i) {
        filter_row->add(std::make_unique<Button>(kResourceFilters[i], [&lab, i] {
            lab.resource.filter = i;
            if (lab.resource.list) lab.resource.list->set_extension_filter(kResourceExtensions[i]);
            if (lab.resource.rebuild_crumbs) lab.resource.rebuild_crumbs();
            if (lab.resource.refresh_info) lab.resource.refresh_info();
        }));
    }
    stage->add(std::move(filter_row));

    // 列表 + 详情。
    auto body = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 820.0f, 372.0f);
    body->set_spacing(12.0f);

    auto list = std::make_unique<xel::ui::FileBrowser>(glm::vec2{560.0f, 372.0f});
    res.list = list.get();
    // FileBrowser 的 measure 会吃掉父给的宽度：不锁尺寸的话详情栏会被挤没。
    res.list->set_min_size({560.0f, 372.0f});
    res.list->set_max_size({560.0f, 372.0f});
    std::error_code dir_ec;
    const std::filesystem::path cwd = std::filesystem::current_path(dir_ec);
    res.list->set_directory(dir_ec ? std::string{"."} : cwd.string());
    res.list->set_extension_filter(kResourceExtensions[res.filter]);

    auto detail = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 240.0f, 372.0f);
    detail->set_spacing(4.0f);
    detail->set_cross_align(xel::ui::Align::top_left());
    detail->add(std::make_unique<Label>("Details", 18.0f));
    auto name = std::make_unique<Label>("-", 18.0f);
    res.info_name = name.get();
    detail->add(std::move(name));
    auto size = std::make_unique<Label>("", 18.0f);
    res.info_size = size.get();
    detail->add(std::move(size));
    auto extra = std::make_unique<Label>("", 18.0f);
    res.info_extra = extra.get();
    detail->add(std::move(extra));

    body->add(std::move(list));
    body->add(std::move(detail));
    stage->add(std::move(body));

    // 详情：名字 / 字节数（目录不统计）/ 图片尺寸（只读头部）。
    res.refresh_info = [&lab] {
        Resource& r = lab.resource;
        if (r.list == nullptr) return;
        const std::string& name = r.list->selected_name();
        if (name.empty()) {
            if (r.info_name) r.info_name->set_text("-");
            if (r.info_size) r.info_size->set_text("");
            if (r.info_extra) r.info_extra->set_text("");
            return;
        }
        const bool is_dir = r.list->selected_is_directory();
        if (r.info_name) r.info_name->set_text(name + (is_dir ? "/" : ""));
        if (r.info_size) {
            r.info_size->set_text(
                is_dir ? std::string{"directory"}
                       : format_bytes(r.list->entry_size(
                             static_cast<std::size_t>(r.list->selected_index()))));
        }
        std::string extra;
        const std::string path = r.list->selected_path();
        if (!is_dir && path.size() > 4 && path.compare(path.size() - 4, 4, ".png") == 0) {
            int w = 0;
            int h = 0;
            int channels = 0;
            if (stbi_info(path.c_str(), &w, &h, &channels) != 0) {
                extra = std::format("{} × {} px", w, h);
            }
        }
        if (r.info_extra) r.info_extra->set_text(extra);
    };

    // 面包屑重建：双击目录 / Backspace 也会改目录，必须跟着刷新。
    res.rebuild_crumbs = [&lab] {
        Resource& r = lab.resource;
        if (r.crumbs == nullptr || r.list == nullptr) return;
        r.crumbs->clear_children();
        r.crumbs->add(std::make_unique<Button>("Up", [&lab] {
            if (lab.resource.list == nullptr) return;
            const std::filesystem::path current(lab.resource.list->directory());
            if (!current.has_parent_path() || current.parent_path() == current) return;
            lab.resource.list->set_directory(current.parent_path().string());
            if (lab.resource.rebuild_crumbs) lab.resource.rebuild_crumbs();
            if (lab.resource.refresh_info) lab.resource.refresh_info();
        }));

        const std::filesystem::path current(r.list->directory());
        std::vector<std::string> parts;
        for (const auto& part : current) parts.push_back(part.string());
        const int start = std::max(0, static_cast<int>(parts.size()) - 6);
        for (int i = start; i < static_cast<int>(parts.size()); ++i) {
            std::filesystem::path target;
            for (int j = 0; j <= i; ++j) target /= parts[static_cast<std::size_t>(j)];
            const std::string segment = parts[static_cast<std::size_t>(i)];
            r.crumbs->add(std::make_unique<Button>(segment.empty() ? std::string{"/"} : segment,
                                                   [&lab, target] {
                if (lab.resource.list == nullptr) return;
                lab.resource.list->set_directory(target.string());
                if (lab.resource.rebuild_crumbs) lab.resource.rebuild_crumbs();
                if (lab.resource.refresh_info) lab.resource.refresh_info();
            }));
        }
    };

    res.list->set_on_select([&lab] {
        if (lab.resource.refresh_info) lab.resource.refresh_info();
    });
    res.list->set_on_activate([&lab] {
        if (lab.resource.refresh_info) lab.resource.refresh_info();
    });
    res.list->set_on_directory_changed([&lab] {
        if (lab.resource.rebuild_crumbs) lab.resource.rebuild_crumbs();
        if (lab.resource.refresh_info) lab.resource.refresh_info();
    });

    res.rebuild_crumbs();
    res.refresh_info();
}

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
        if (window != nullptr) window->set_visible(index == 7);
    }

    if (stage == nullptr) return;
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
    // E7 资源浏览器：从 cosine 搬来的骨架 + 面包屑 / 过滤 / 详情 / 键盘。
    if (index == 6) {
        build_resource(*this, stage);
        set_status("E7 资源浏览器：面包屑、类型过滤、条目信息、键盘导航");
        return;
    }

    // E8 可拖动窗口：窗口已在 build() 里挂到 root，这里只控制显隐。
    if (index == 7) {
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

    auto tabs = std::make_unique<xel::ui::TabBar>();
    tabs->add_tab("Font");
    tabs->add_tab("Theme");
    tabs->add_tab("Scene");
    tabs->set_size(426.0f, 36.0f);
    auto* tabs_ptr = tabs.get();
    settings->add(std::move(tabs));

    auto settings_status = std::make_unique<Label>("font debug: off", 18.0f);
    auto* settings_status_ptr = settings_status.get();
    settings->add(std::move(settings_status));

    auto section = std::make_unique<xel::ui::Section>("Font");
    auto* section_ptr = section.get();
    settings->add(std::move(section));

    // 三页内容切换时重建（Section::clear + add），避免隐藏页仍参与排布。
    auto fill_font = [&lab, section_ptr, settings_status_ptr] {
        section_ptr->set_title("Font · 文字与调试绘制");
        section_ptr->clear();
        section_ptr->add(std::make_unique<Button>("Toggle font debug", [&lab, settings_status_ptr] {
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
        section_ptr->add(std::make_unique<Button>("Glyph boxes only", [&lab] {
            xel::graphics::TextDebugOptions options;
            options.glyph_boxes = true;
            apply_label_debug(lab.root, options);
            lab.set_status("font debug: glyph boxes");
        }));
        section_ptr->add(std::make_unique<Label>(
            "调试绘制作用于现有控件，新建控件需再点一次", 18.0f));
    };

    auto fill_theme = [&lab, section_ptr] {
        section_ptr->set_title("Theme · 主题与密度");
        section_ptr->clear();
        section_ptr->add(std::make_unique<Label>(
            "text " + std::format("{:.0f}", lab.theme.text_size) + "    control " +
                std::format("{:.0f}", lab.theme.control_height) + "    padding " +
                std::format("{:.0f}", lab.theme.padding) + "    spacing " +
                std::format("{:.0f}", lab.theme.spacing),
            18.0f));
        section_ptr->add(std::make_unique<Label>("密度实时调参：见 E2 主题密度矩阵", 18.0f));
        section_ptr->add(std::make_unique<Label>("令牌色板编辑：待 UI-3 后续", 18.0f));
    };

    auto fill_scene = [&lab, section_ptr] {
        section_ptr->set_title("Scene · 场景与导出");
        section_ptr->clear();
        section_ptr->add(std::make_unique<Label>(
            std::string{"current: "} + kExperiments[lab.current < 0 ? 0 : lab.current].title,
            18.0f));
        section_ptr->add(std::make_unique<Button>("Next experiment", [&lab] {
            lab.select((lab.current + 1) % kExperimentCount);
        }));
        section_ptr->add(std::make_unique<Label>("离屏导出：见 E1 的 Export PNG", 18.0f));
    };

    std::vector<std::function<void()>> fills{fill_font, fill_theme, fill_scene};
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
    fill_font();

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

int run()
{
    xel::Application app{static_cast<int>(kW), static_cast<int>(kH), "ui_lab"};

    Lab lab;
    build(lab, *app.uisys.root());

    // 离屏导出：与其它示例一致的固定尺寸，便于人工核对布局。
    if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
        app.uisys.tick(1.0f / 60.0f);

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

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();

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
