#include "demo.hpp"
#include "sections/sections.hpp"

#include "ui/composites/menu_bar.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/containers/panel.hpp"
#include "ui/core/button.hpp"
#include "ui/core/spacer.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string_view>

namespace demo
{
namespace
{
// ── 分区注册表（唯一扩展点）───────────────────────────────────────
const std::vector<Section>& registry()
{
    static const std::vector<Section> kSections = {
        {"overview", "概览", "这个演示台有什么、怎么操作，以及运行指标。",
         &sections::build_overview},
        {"basics", "基础控件", "按钮 / 勾选 / 单选 / 标签 / 进度 / 装饰元素。",
         &sections::build_basics},
        {"inputs", "输入控件", "文本、数值拖拽、颜色、滑杆、下拉，含联动回显。",
         &sections::build_inputs},
        {"text", "文本与字体", "标题层级、对齐、换行、省略、字号与行距。",
         &sections::build_text},
        {"layout", "布局", "Vbox/Hbox/Grid/Wrap 的间距、对齐、flex、百分比、绝对定位。",
         &sections::build_layout},
        {"containers", "容器与视图", "TabView / Section / ScrollPanel / Splitter 组合。",
         &sections::build_containers},
        {"data", "数据视图", "Table 与 TreeView：滚动、斑马纹、选中、展开。",
         &sections::build_data},
        {"overlays", "浮层与模态", "Window / Dialog / MenuBar / Dropdown / Tooltip 的层叠。",
         &sections::build_overlays},
        {"media", "媒体与时间", "Image 占位与纹理、PreviewRuler 拖动定位、颜色色板。",
         &sections::build_media},
    };
    return kSections;
}

ui::Theme compose_theme(bool dark, ui::Density density, ui::FocusPolicy policy, bool rounded,
                        bool transitions)
{
    ui::Theme theme = dark ? ui::Theme::dark() : ui::Theme::light();
    theme = theme.with_density(density);
    theme = theme.with_focus_policy(policy);
    theme = theme.with_rounding(rounded);
    theme = theme.with_transition_ms(transitions ? 140.0f : 0.0f);
    return theme;
}

const char* focus_policy_name(ui::FocusPolicy policy)
{
    switch (policy) {
        case ui::FocusPolicy::Keyboard: return "Keyboard";
        case ui::FocusPolicy::Always:   return "Always";
        case ui::FocusPolicy::Never:    return "Never";
    }
    return "?";
}

const char* density_name(ui::Density density)
{
    switch (density) {
        case ui::Density::Compact:  return "Compact";
        case ui::Density::Normal:   return "Normal";
        case ui::Density::Spacious: return "Spacious";
    }
    return "?";
}
} // namespace

// ── 注册表 ────────────────────────────────────────────────────────
const std::vector<Section>& section_list()
{
    return registry();
}

// ── 构建辅助 ──────────────────────────────────────────────────────
ui::Paragraph& hint(ui::Widget& parent, std::string text)
{
    auto label = std::make_unique<ui::Paragraph>(std::move(text));
    auto& ref = *label;
    ref.set_text_size(ref.ui_theme().small_text_size + 1.0f);
    ref.set_text_color(ref.ui_theme().text_muted);
    ref.set_fill(true, false);
    parent.add(std::move(label));
    return ref;
}

ui::Hbox& row(ui::Widget& parent, float spacing)
{
    auto box = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 0.0f);
    auto& ref = *box;
    ref.set_spacing(spacing);
    ref.set_cross_align(ui::Align::center_left());
    ref.set_fill(true, false);
    parent.add(std::move(box));
    return ref;
}

ui::Vbox& group(ui::Vbox& column, std::string title)
{
    // 卡片：surface 底 + 标题行 + 内容列；没有折叠交互。
    auto card = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 0.0f);
    card->set_padding(10.0f);
    card->set_spacing(8.0f);
    card->set_cross_align(ui::Align::top_left());
    card->set_fill(true, false);

    auto head = std::make_unique<ui::Label>(std::move(title));
    head->set_text_size(head->ui_theme().small_text_size + 3.0f);
    head->set_text_color(head->ui_theme().text);
    card->add(std::move(head));

    auto& ref = *card;
    column.add(std::move(card));
    return ref;
}

void gap(ui::Widget& parent, float height)
{
    auto spacer = std::make_unique<ui::Spacer>(0.0f, height);
    spacer->set_fill(true, false);
    parent.add(std::move(spacer));
}


// ── Demo ──────────────────────────────────────────────────────────
Demo::Demo(ui::UIRoot& root, std::function<void(float)> set_ui_scale)
: root_{&root}, set_ui_scale_(std::move(set_ui_scale))
{
    build_shell();
    apply_theme();

    // COSINE_DEMO_SECTION 可直达某一分区（截图/冒烟用）；缺省进第一个。
    if (const char* id = std::getenv("COSINE_DEMO_SECTION")) {
        show_section(std::string{id});
    } else {
        show_section(0);
    }
}

Demo::~Demo() = default;

const Section& Demo::section() const
{
    const auto& list = section_list();
    const int index = std::clamp(current_, 0, static_cast<int>(list.size()) - 1);
    return list[static_cast<std::size_t>(index)];
}

int Demo::built_page_count() const
{
    return static_cast<int>(std::count(page_built_.begin(), page_built_.end(), true));
}

// ── 外壳 ──────────────────────────────────────────────────────────
void Demo::build_shell()
{
    // 根：一列（顶栏 / 标签页 + 内容 / 状态栏）。
    auto column = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    column->set_spacing(0.0f);
    column->set_padding(0.0f);
    column->set_cross_align(ui::Align::h_stretch());
    column->set_fill(true, true);
    auto* column_ptr = column.get();
    root_->add(std::move(column));

    build_top_bar(*column_ptr);

    // 第二行起是标签页导航：每个分区一个标签 + 可滚动页面。
    auto tabs = std::make_unique<ui::TabView>();
    tabs->set_flex_grow(1.0f);
    tabs->set_fill(true, true);
    tabs->set_page_fill(true);
    auto* tabs_ptr = tabs.get();

    const auto& list = section_list();
    page_columns_.assign(list.size(), nullptr);
    page_built_.assign(list.size(), false);

    for (std::size_t i = 0; i < list.size(); ++i) {
        auto page = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
        page->set_padding(14.0f);
        page->content()->set_spacing(12.0f);
        page->content()->set_cross_align(ui::Align::h_stretch());
        // 内容列地址在 unique_ptr 移动后依旧稳定（堆上节点），先存下来。
        page_columns_[i] = page->content();
        tabs_ptr->add_page(list[i].title, std::move(page));
    }

    tabs->set_on_change([this](int index) { ensure_page(index); });
    shell_.tabs = tabs_ptr;
    column_ptr->add(std::move(tabs));

    build_status_bar(*column_ptr);
}

void Demo::build_top_bar(ui::Vbox& column)
{
    auto bar = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 0.0f);
    bar->set_spacing(6.0f);
    bar->set_cross_align(ui::Align::center_left());
    bar->set_padding(ui::Edges{6.0f, 10.0f, 6.0f, 10.0f});
    bar->set_fill(true, false);
    auto& bar_ref = *bar;

    // 菜单：每一项都作用到这个演示台本身（主题 / 分区 / 反馈）。
    auto menu = std::make_unique<ui::MenuBar>();
    menu->set_flex_grow(1.0f);
    auto* menu_raw = menu.get();
    bar_ref.add(std::move(menu));

    std::vector<ui::MenuItem> view_items;
    for (std::size_t i = 0; i < section_list().size(); ++i) {
        const auto& section = section_list()[i];
        view_items.push_back(ui::MenuItem::make(
            section.title, [this, i] { show_section(static_cast<int>(i)); },
            i < 9 ? std::to_string(i + 1) : std::string{}));
    }

    std::vector<ui::MenuItem> theme_items{
        ui::MenuItem::make("切换 暗色 / 亮色", [this] { toggle_dark(); }, "T"),
        ui::MenuItem::make("圆角 / 直角", [this] { toggle_rounded(); }),
        ui::MenuItem::make("焦点策略循环", [this] { cycle_focus_policy(); }),
        ui::MenuItem::make("密度循环", [this] { cycle_density(); }),
        ui::MenuItem::make("过渡 开 / 关", [this] { toggle_transitions(); }),
        ui::MenuItem::separator_item(),
        ui::MenuItem::submenu("缩放", {
            ui::MenuItem::make("0.75x", [this] { set_ui_scale(0.75f); }),
            ui::MenuItem::make("1.00x", [this] { set_ui_scale(1.0f); }),
            ui::MenuItem::make("1.25x", [this] { set_ui_scale(1.25f); }),
            ui::MenuItem::make("1.50x", [this] { set_ui_scale(1.5f); }),
        }),
    };

    std::vector<ui::MenuItem> help_items{
        ui::MenuItem::make("分区总数: " + std::to_string(section_list().size()), nullptr),
        ui::MenuItem::make("第二行标签页切换分区", nullptr),
        ui::MenuItem::make("Tab / Shift+Tab 切换焦点", nullptr),
        ui::MenuItem::separator_item(),
        ui::MenuItem::make("关于这个演示台", [this] {
            note("XEL UI Demo：注册表驱动的标签页展示台（新增分区只改注册表）");
        }),
    };

    menu_raw->add_menu("视图", std::move(view_items));
    menu_raw->add_menu("主题", std::move(theme_items));
    menu_raw->add_menu("帮助", std::move(help_items));

    // 常用开关直接放在顶栏：不点菜单也能改。
    auto dark = std::make_unique<ui::Button>("暗/亮");
    dark->set_on_click([this] { toggle_dark(); });
    bar_ref.add(std::move(dark));

    auto density = std::make_unique<ui::Button>("密度");
    density->set_on_click([this] { cycle_density(); });
    bar_ref.add(std::move(density));

    auto focus = std::make_unique<ui::Button>("焦点");
    focus->set_on_click([this] { cycle_focus_policy(); });
    bar_ref.add(std::move(focus));

    column.add(std::move(bar));
}

void Demo::build_status_bar(ui::Vbox& column)
{
    auto bar = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 0.0f);
    bar->set_spacing(12.0f);
    bar->set_cross_align(ui::Align::center_left());
    bar->set_padding(ui::Edges{4.0f, 10.0f, 4.0f, 10.0f});
    bar->set_fill(true, false);

    auto label = std::make_unique<ui::Label>("", root_->ui_theme().small_text_size);
    label->set_flex_grow(1.0f);
    label->set_fill(true, false);
    shell_.status = label.get();
    bar->add(std::move(label));

    column.add(std::move(bar));
}

void Demo::ensure_page(int index)
{
    const auto& list = section_list();
    if (index < 0 || index >= static_cast<int>(list.size())) return;
    if (page_built_[static_cast<std::size_t>(index)]) return;

    auto* column = page_columns_[static_cast<std::size_t>(index)];
    if (column == nullptr) return;

    const Section& section = list[static_cast<std::size_t>(index)];
    auto title = std::make_unique<ui::Heading>(section.title, 1);
    title->set_fill(true, false);
    column->add(std::move(title));
    hint(*column, section.summary);
    gap(*column, 2.0f);

    if (section.build != nullptr) section.build(*this, *column);
    page_built_[static_cast<std::size_t>(index)] = true;
}

void Demo::show_section(int index)
{
    const int count = static_cast<int>(section_list().size());
    if (index < 0 || index >= count) return;

    ensure_page(index);
    if (shell_.tabs != nullptr) shell_.tabs->set_selected(index);
    current_ = index;
}

void Demo::show_section(const std::string& id)
{
    const auto& list = section_list();
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].id == id) {
            show_section(static_cast<int>(i));
            return;
        }
    }
    note("未知分区 id: " + id);
}

// ── 主题 / DPI ────────────────────────────────────────────────────
void Demo::apply_theme()
{
    ui::Theme theme = compose_theme(dark_, density_, focus_policy_, rounded_, transitions_);
    root_->set_theme(theme);
    // 标签页与已建内容都要按新密度重新量一次。
    if (shell_.tabs != nullptr) shell_.tabs->invalidate_measure_recursive();
    for (auto* column : page_columns_) {
        if (column != nullptr) column->invalidate_measure_recursive();
    }
}

void Demo::set_dark(bool dark)
{
    if (dark_ == dark) return;
    dark_ = dark;
    apply_theme();
    note(dark_ ? "主题：暗色" : "主题：亮色");
}

void Demo::toggle_dark()
{
    set_dark(!dark_);
}

void Demo::toggle_rounded()
{
    rounded_ = !rounded_;
    apply_theme();
    note(rounded_ ? "圆角：开" : "圆角：关（直角）");
}

void Demo::cycle_focus_policy()
{
    switch (focus_policy_) {
        case ui::FocusPolicy::Keyboard: focus_policy_ = ui::FocusPolicy::Always; break;
        case ui::FocusPolicy::Always:   focus_policy_ = ui::FocusPolicy::Never; break;
        case ui::FocusPolicy::Never:    focus_policy_ = ui::FocusPolicy::Keyboard; break;
    }
    apply_theme();
    note(std::string{"焦点策略："} + focus_policy_name(focus_policy_));
}

void Demo::set_density(ui::Density density)
{
    if (density_ == density) return;
    density_ = density;
    apply_theme();
    note(std::string{"密度："} + density_name(density_));
}

void Demo::cycle_density()
{
    switch (density_) {
        case ui::Density::Compact:  set_density(ui::Density::Normal); break;
        case ui::Density::Normal:   set_density(ui::Density::Spacious); break;
        case ui::Density::Spacious: set_density(ui::Density::Compact); break;
    }
}

void Demo::toggle_transitions()
{
    transitions_ = !transitions_;
    apply_theme();
    note(transitions_ ? "过渡：140 ms" : "过渡：关闭（状态瞬间切换）");
}

void Demo::set_ui_scale(float scale)
{
    const float value = scale > 0.0f ? scale : 1.0f;
    if (!set_ui_scale_) {
        note("当前运行方式不支持缩放切换");
        return;
    }
    set_ui_scale_(value);
    ui_scale_ = value;
    char text[64];
    std::snprintf(text, sizeof(text), "UI 缩放：%.2fx", static_cast<double>(value));
    note(text);
}

// ── 反馈与每帧 ────────────────────────────────────────────────────
void Demo::note(std::string text)
{
    note_ = std::move(text);
}

void Demo::update(float /*dt*/, const graphics::FrameStats& stats, ui::Widget* hovered,
                  ui::Widget* focused)
{
    if (shell_.status == nullptr) return;

    const Section& current = section();
    const auto name_of = [](ui::Widget* widget) -> std::string {
        if (widget == nullptr) return "-";
        const std::string_view label = widget->semantics().label;
        return label.empty() ? std::string{"未命名"} : std::string{label};
    };

    char buffer[512];
    std::snprintf(buffer, sizeof(buffer),
                  "%s  │  fps %.0f  frame %.2f ms  draw %u  verts %u  │  焦点 %s  悬停 %s  │  %s",
                  current.title.c_str(), static_cast<double>(stats.fps),
                  static_cast<double>(stats.frame_time_ms),
                  static_cast<unsigned>(stats.draw.draw_calls),
                  static_cast<unsigned>(stats.draw.vertices), name_of(focused).c_str(),
                  name_of(hovered).c_str(), note_.c_str());
    shell_.status->set_text(buffer);
}

// ── 工厂 ──────────────────────────────────────────────────────────
std::unique_ptr<Demo> create(ui::UIRoot& root, std::function<void(float)> set_ui_scale)
{
    return std::make_unique<Demo>(root, std::move(set_ui_scale));
}
} // namespace demo
