// 基础控件 / 输入控件 / 文本与字体 三个分区。
#include "sections/sections.hpp"

#include "ui/composites/dropdown.hpp"
#include "ui/composites/radio_group.hpp"
#include "ui/containers/grid.hpp"
#include "ui/core/button.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/color_input.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/drag_float.hpp"
#include "ui/core/label.hpp"
#include "ui/core/progress.hpp"
#include "ui/core/radio.hpp"
#include "ui/core/slider.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/tag.hpp"
#include "ui/core/text_input.hpp"
#include "ui/core/typography.hpp"

#include <string>
#include <vector>

namespace demo::sections
{
namespace
{
ui::Label& live_label(ui::Widget& parent, std::string text, float size = 0.0f)
{
    auto label = std::make_unique<ui::Label>(std::move(text), size);
    auto& ref = *label;
    parent.add(std::move(label));
    return ref;
}

ui::Button& add_button(ui::Hbox& row, Demo& demo, std::string label)
{
    auto button = std::make_unique<ui::Button>(label);
    auto& ref = *button;
    ref.set_on_click([&demo, text = std::move(label)] { demo.note("按钮点击：" + text); });
    row.add(std::move(button));
    return ref;
}
} // namespace

// ════════════════════════════════════════════════════════════════════
// 概览：说明 + 操作表 + 运行指标
// ════════════════════════════════════════════════════════════════════
void build_overview(Demo& demo, ui::Vbox& column)
{
    auto& intro = group(column, "这是什么");
    hint(intro,
         "XEL UI Demo 是一个「注册表驱动」的展示台：第二行的每个标签页由一条 Section 记录"
         "（id / 标题 / 说明 / build 函数）生成，外壳（顶栏、标签页、状态栏）与内容完全解耦。"
         "新增内容只需要在注册表里加一条，不需要改外壳代码。");
    hint(intro,
         "标签页是懒构建的：第一次点开时才建树，之后靠页签切换，所以来回切分区会保留"
         "各自的滚动位置、输入内容与选中状态。");

    auto& ops = group(column, "操作方式");
    auto& keys = row(ops);
    auto table = []() -> std::unique_ptr<ui::Grid> {
        auto grid = std::make_unique<ui::Grid>(0.0f, 0.0f, 0.0f, 0.0f);
        grid->set_columns(2);
        grid->set_spacing(4.0f, 4.0f);
        grid->set_fill(true, false);
        return grid;
    }();
    const std::pair<const char*, const char*> rows[] = {
        {"第二行标签页 / 视图菜单", "切换分区（菜单里带数字快捷键提示）"},
        {"顶栏三个按钮", "快速切换 暗/亮、密度、焦点策略"},
        {"主题菜单", "圆角、焦点策略、密度、过渡时长、UI 缩放"},
        {"Tab / Shift+Tab", "键序遍历当前分区内的可聚焦控件"},
        {"页内滚轮", "当前页独立滚动；切走再回来位置保留"},
    };
    for (const auto& [left, right] : rows) {
        auto key = std::make_unique<ui::Tag>(left);
        table->add(std::move(key));
        table->add(std::make_unique<ui::Label>(right, 0.0f));
    }
    keys.add(std::move(table));

    auto& live = group(column, "运行指标");
    hint(live, "底部状态栏每帧刷新 fps / 帧耗时 / draw call / 顶点数 / 焦点 / 悬停 / 最近交互。");
    auto& metrics = row(live);
    auto indicator = std::make_unique<ui::Progress>(160.0f);
    auto* indicator_ptr = indicator.get();
    metrics.add(std::move(indicator));
    metrics.add(std::make_unique<ui::Tag>("状态栏实时刷新"));
    // 进度条随密度/主题切换演示：值来自 ui_scale（1.0 → 0.25）。
    indicator_ptr->set_value(0.35f + demo.ui_scale() * 0.2f);

    auto& theme_state = group(column, "当前外观");
    auto* state = &live_label(theme_state, "");
    state->set_text(demo.dark() ? "暗色主题" : "亮色主题");
}

// ════════════════════════════════════════════════════════════════════
// 基础控件：按钮 / 勾选 / 单选 / 标签 / 进度 / 装饰
// ════════════════════════════════════════════════════════════════════
void build_basics(Demo& demo, ui::Vbox& column)
{
    auto& buttons = group(column, "按钮 Button");
    hint(buttons, "普通按钮、带回调的按钮、禁用状态（禁用时点击与键盘都不触发）。");
    auto& button_row = row(buttons);
    add_button(button_row, demo, "普通按钮");
    add_button(button_row, demo, "再点一次");
    auto disabled = std::make_unique<ui::Button>("禁用按钮");
    disabled->set_enabled(false);
    button_row.add(std::move(disabled));

    auto& checks = group(column, "勾选 Checkbox");
    hint(checks, "可勾选、可禁用；勾选状态下 Button 角色会带 checked 语义。");
    auto& check_row = row(checks);
    auto check = std::make_unique<ui::Checkbox>("启用网格吸附", false);
    auto* check_ptr = check.get();
    check_row.add(std::move(check));
    auto& check_live = live_label(check_row, "checked = false");
    check_ptr->set_on_change([&demo, &check_live](bool on) {
        check_live.set_text(on ? "checked = true" : "checked = false");
        demo.note(std::string{"勾选："} + (on ? "开" : "关"));
    });
    auto check_disabled = std::make_unique<ui::Checkbox>("禁用勾选", true);
    check_disabled->set_enabled(false);
    check_row.add(std::move(check_disabled));

    auto& radios = group(column, "单选 Radio / RadioGroup");
    hint(radios, "单个 Radio 互斥由容器保证；RadioGroup 提供选项列与键盘上下切换。");
    auto& radio_row = row(radios);
    auto group_a = std::make_unique<ui::RadioGroup>(180.0f);
    group_a->set_align(ui::Align::top_left());
    group_a->set_options({"线性布局", "网格布局", "流式布局"}, 0);
    group_a->set_on_change([&demo](int index) {
        demo.note("RadioGroup 选中项：" + std::to_string(index));
    });
    radio_row.add(std::move(group_a));

    auto standalone = std::make_unique<ui::RadioGroup>(160.0f);
    standalone->set_align(ui::Align::top_left());
    standalone->set_options({"单选项 A", "单选项 B"}, 1);
    radio_row.add(std::move(standalone));

    auto& tags = group(column, "标签 Tag / 装饰");
    hint(tags, "Tag 是「可点的叶子」：有焦点环、有语义角色，可作为轻量入口。");
    auto& tag_row = row(tags);
    tag_row.add(std::make_unique<ui::Tag>("点我"));
    tag_row.add(std::make_unique<ui::Tag>("中文标签"));
    tag_row.add(std::make_unique<ui::Tag>("键盘可达"));
    auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 60.0f, 22.0f,
                                                glm::vec4{0.25f, 0.55f, 0.9f, 1.0f});
    tag_row.add(std::move(rect));
    tag_row.add(std::make_unique<ui::Spacer>(12.0f, 22.0f));
    tag_row.add(std::make_unique<ui::Label>("↑ ColorRect + Spacer"));

    auto& progress = group(column, "进度 Progress");
    hint(progress, "确定进度按值填充；不确定进度用一个往返的相位动画（每帧推进）。");
    auto& progress_row = row(progress);
    auto determinate = std::make_unique<ui::Progress>(220.0f);
    determinate->set_value(0.42f);
    progress_row.add(std::move(determinate));
    auto indeterminate = std::make_unique<ui::Progress>(180.0f);
    indeterminate->set_indeterminate(true);
    progress_row.add(std::move(indeterminate));
    progress_row.add(std::make_unique<ui::Label>("42% / 不确定"));
}

// ════════════════════════════════════════════════════════════════════
// 输入控件：文本 / 拖拽数值 / 颜色 / 滑杆 / 下拉
// ════════════════════════════════════════════════════════════════════
void build_inputs(Demo& demo, ui::Vbox& column)
{
    auto& text = group(column, "文本输入 TextInput");
    hint(text, "单行与多行；改动会即时回显到右侧标签。");
    auto& text_row = row(text);
    auto& echo = live_label(text_row, "echo: （空）");
    auto input = std::make_unique<ui::TextInput>("hello", 220.0f);
    input->set_on_change([&echo](const std::string& value) {
        echo.set_text("echo: " + (value.empty() ? std::string{"（空）"} : value));
    });
    text_row.add(std::move(input));
    auto multiline = std::make_unique<ui::TextInput>("第一行\n第二行", 200.0f);
    multiline->set_multiline(true);
    multiline->set_max_height(90.0f);
    text_row.add(std::move(multiline));

    auto& numeric = group(column, "数值 DragFloat / Slider");
    hint(numeric, "DragFloat 左右拖动改值，支持范围/步长/精度/后缀；Slider 可键盘微调。");
    auto& numeric_row = row(numeric);
    auto drag = std::make_unique<ui::DragFloat>("宽", 160.0f);
    drag->set_range(0.0f, 500.0f);
    drag->set_step(1.0f);
    drag->set_precision(0);
    drag->set_suffix(" px");
    drag->set_value(240.0f);
    numeric_row.add(std::move(drag));

    auto slider = std::make_unique<ui::Slider>(200.0f);
    slider->set_range(0.0f, 100.0f);
    slider->set_step(5.0f);
    slider->set_value(60.0f);
    auto& slider_echo = live_label(numeric_row, "slider = 60");
    slider->set_on_change([&slider_echo](float value) {
        char text_value[32];
        std::snprintf(text_value, sizeof(text_value), "slider = %.0f", static_cast<double>(value));
        slider_echo.set_text(text_value);
    });
    numeric_row.add(std::move(slider));

    auto& color = group(column, "颜色 ColorInput");
    hint(color, "点击展开调色板，选项即提交；面板本身是浮动层。");
    auto& color_row = row(color);
    auto picker = std::make_unique<ui::ColorInput>("强调色", 240.0f);
    picker->set_palette({
        {0.24f, 0.56f, 0.91f, 1.0f},
        {0.91f, 0.44f, 0.32f, 1.0f},
        {0.36f, 0.78f, 0.51f, 1.0f},
        {0.85f, 0.76f, 0.35f, 1.0f},
        {0.66f, 0.47f, 0.87f, 1.0f},
    });
    picker->set_on_change([&demo](glm::vec4 c) {
        char text_value[96];
        std::snprintf(text_value, sizeof(text_value), "颜色：%.2f, %.2f, %.2f",
                      static_cast<double>(c.r), static_cast<double>(c.g),
                      static_cast<double>(c.b));
        demo.note(text_value);
    });
    color_row.add(std::move(picker));
    auto swatch = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 36.0f, 24.0f,
                                                  glm::vec4{0.24f, 0.56f, 0.91f, 1.0f});
    color_row.add(std::move(swatch));

    auto& dropdown = group(column, "下拉 Dropdown");
    hint(dropdown, "弹出列表是 Floating 层（挂在 root，逃逸面板裁剪）；键盘上下选择。");
    auto& dropdown_row = row(dropdown);
    auto select = std::make_unique<ui::Dropdown>(
        std::vector<std::string>{"Apple", "Banana", "Cherry", "Dragon fruit"}, 0);
    auto& select_echo = live_label(dropdown_row, "selected = 0");
    select->set_on_change([&select_echo, &demo](int index) {
        select_echo.set_text("selected = " + std::to_string(index));
        demo.note("下拉选中：" + std::to_string(index));
    });
    dropdown_row.add(std::move(select));
}

// ════════════════════════════════════════════════════════════════════
// 文本与字体
// ════════════════════════════════════════════════════════════════════
void build_text(Demo& demo, ui::Vbox& column)
{
    auto& headings = group(column, "标题层级");
    headings.add(std::make_unique<ui::Heading>("一级标题 Heading(1)", 1));
    headings.add(std::make_unique<ui::Heading>("二级标题 Heading(2)", 2));
    headings.add(std::make_unique<ui::Heading>("三级标题 Heading(3)", 3));

    auto& body = group(column, "正文与说明");
    hint(body,
         "Paragraph 默认开启换行，跟随容器宽度重排；Label 可以单独控制字号、颜色、行距与省略。");
    auto wrap = std::make_unique<ui::Paragraph>(
        "这是一段用于演示换行的长文本：XEL 的文本渲染按字形栅格化，字号与 UI 缩放都会"
        "触发重新栅格化，所以放大界面不会出现位图模糊。切换顶栏的密度按钮，可以看到"
        "字号与控件高度一起变化。");
    wrap->set_fill(true, false);
    body.add(std::move(wrap));

    auto& align = group(column, "对齐与省略");
    const ui::TextAlign aligns[] = {ui::TextAlign::Left, ui::TextAlign::Center, ui::TextAlign::Right};
    const char* names[] = {"Left", "Center", "Right"};
    for (int i = 0; i < 3; ++i) {
        auto label = std::make_unique<ui::Label>(std::string{"对齐 "} + names[i] + " —— 0123456789",
                                                 0.0f);
        label->set_fill(true, false);
        label->set_text_align(aligns[i]);
        align.add(std::move(label));
    }
    auto ellipsis = std::make_unique<ui::Label>(
        "这一行很长，用来演示省略号：XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0.0f);
    ellipsis->set_fill(true, false);
    ellipsis->set_max_lines(1);
    ellipsis->set_ellipsis(true);
    align.add(std::move(ellipsis));

    auto& size = group(column, "字号联动");
    hint(size, "拖动滑杆改字号，预览立刻按新字号重新栅格化（不是缩放位图）。");
    auto& size_row = row(size);
    auto preview = std::make_unique<ui::Label>("预览：中文 ABC 123", 28.0f);
    preview->set_flex_grow(1.0f);
    preview->set_fill(true, false);
    auto* preview_ptr = preview.get();
    size_row.add(std::move(preview));

    auto slider = std::make_unique<ui::Slider>(200.0f);
    slider->set_range(10.0f, 64.0f);
    slider->set_step(1.0f);
    slider->set_value(28.0f);
    slider->set_on_change([preview_ptr, &demo](float value) {
        preview_ptr->set_text_size(value);
        char text_value[48];
        std::snprintf(text_value, sizeof(text_value), "字号：%.0f", static_cast<double>(value));
        demo.note(text_value);
    });
    size_row.add(std::move(slider));
}
} // namespace demo::sections
