// 高级 UI 实验室（EX-2）：探索**未定 API** 的组合控件。
// 与 ui_gallery 的分工、纪律与结论表见同目录 README.md。
// 硬约束：产品代码不得依赖本目录；lab 不进 ctest 断言，只有冒烟门禁。

#include "ui/button.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/root.hpp"
#include "ui/slider.hpp"
#include "ui/text_input.hpp"
#include "ui/theme.hpp"
#include "ui/vbox.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

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
};
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
        if (shown == 0) list->add(std::make_unique<Label>("no matching command", 13.0f));
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
    xel::ui::Theme theme;
    int current = -1;

    void set_status(std::string text)
    {
        if (status != nullptr) status->set_text(std::move(text));
    }

    void select(int index);
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

    if (stage == nullptr) return;
    stage->clear_children();
    palette = Palette{};
    density = Density{};

    if (title != nullptr) title->set_text(kExperiments[index].title);
    if (hypothesis != nullptr) {
        hypothesis->set_text(std::string{"假设："} + kExperiments[index].hypothesis);
    }

    if (!kExperiments[index].implemented) {
        stage->add(std::make_unique<Label>("该实验尚未实现（见 README 的实验清单）", 14.0f));
        set_status(kExperiments[index].title);
        return;
    }

    if (index == 0) {
        stage->add(std::make_unique<Label>("输入过滤命令，点击执行：", 14.0f));

        auto input = std::make_unique<TextInput>("", 560.0f);
        palette.input = input.get();
        stage->add(std::move(input));

        auto list = std::make_unique<Vbox>(0.0f, 0.0f, 700.0f, 380.0f);
        list->set_spacing(4.0f);
        palette.list = list.get();
        stage->add(std::move(list));

        auto result = std::make_unique<Label>("尚未执行任何命令", 14.0f);
        palette.result = result.get();
        stage->add(std::move(result));

        palette.input->set_on_change([this](const std::string&) { palette.rebuild(); });
        palette.rebuild();
        set_status("E1 命令面板：输入以过滤，点击执行");
        return;
    }

    // E2：四个标量 + 应用按钮。
    stage->add(std::make_unique<Label>("拖动后点 Apply，观察既有布局如何反应：", 14.0f));

    auto add_row = [this](const char* name, float lo, float hi, float initial,
                          Slider** slider_out, Label** value_out) {
        auto row = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 640.0f, 34.0f);
        row->set_spacing(10.0f);
        row->set_cross_align(xel::ui::Align::center_left());
        row->add(std::make_unique<Label>(name, 14.0f));

        auto slider = std::make_unique<Slider>(300.0f);
        slider->set_range(lo, hi);
        slider->set_value(initial);
        *slider_out = slider.get();

        auto value = std::make_unique<Label>(std::format("{:.0f}", initial), 13.0f);
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

void build(Lab& lab, xel::ui::UIRoot& root)
{
    lab.root = &root;
    lab.theme = xel::ui::Theme::dark();
    lab.theme.text_size = 16.0f;
    lab.theme.small_text_size = 13.0f;
    lab.theme.heading_text_size = 22.0f;
    lab.theme.control_height = 32.0f;
    root.set_theme(lab.theme);

    auto heading = std::make_unique<Label>("ui_lab · 高级 UI 实验室", 22.0f);
    heading->set_pos(16.0f, 10.0f);
    root.add(std::move(heading));

    auto note = std::make_unique<Label>("实验代码，产品不依赖；结论回填 examples/ui_lab/README.md",
                                        13.0f);
    note->set_pos(16.0f, 38.0f);
    root.add(std::move(note));

    auto left = std::make_unique<Panel>(kLeftX, kColumnY, kLeftW, kColumnH);
    left->set_cross_align(xel::ui::Align::h_stretch());
    left->add(std::make_unique<Label>("Experiments", 15.0f));
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

    auto hypothesis = std::make_unique<Label>("", 13.0f);
    lab.hypothesis = hypothesis.get();
    right->add(std::move(hypothesis));

    auto stage = std::make_unique<Vbox>(0.0f, 0.0f, kRightW - 24.0f, 500.0f);
    stage->set_spacing(6.0f);
    stage->set_cross_align(xel::ui::Align::top_left());
    lab.stage = stage.get();
    right->add(std::move(stage));
    root.add(std::move(right));

    auto status = std::make_unique<Label>("ready", 13.0f);
    status->set_pos(16.0f, kH - 26.0f);
    lab.status = status.get();
    root.add(std::move(status));

    lab.select(0);
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
