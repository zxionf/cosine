#pragma once

#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/color_rect.hpp"
#include "ui/dropdown.hpp"
#include "ui/debug_overlay.hpp"
#include "ui/dialog.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/menu_bar.hpp"
#include "ui/list.hpp"
#include "ui/panel.hpp"
#include "ui/progress.hpp"
#include "ui/radio_group.hpp"
#include "ui/root.hpp"
#include "ui/scroll_panel.hpp"
#include "ui/scroll_view.hpp"
#include "ui/slider.hpp"
#include "ui/tab_bar.hpp"
#include "ui/table.hpp"
#include "ui/tab_view.hpp"
#include "ui/tree_view.hpp"
#include "ui/spacer.hpp"
#include "ui/splitter.hpp"
#include "ui/text_input.hpp"
#include "ui/tooltip.hpp"
#include "ui/ui_system.hpp"
#include "ui/vbox.hpp"
#include "xel/version.hpp"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <vector>
#include <cstdio>

namespace xel::demo
{
// UI gallery：分区展示当前所有控件与交互能力。
// 左侧导航切换分区，右侧为当前分区内容；overlay 层承载 tooltip 与调试面板。
struct UiGallery
{
    static constexpr float kNavWidth = 220.0f;
    static constexpr float kContentX = 236.0f;
    static constexpr float kContentWidth = 940.0f;
    static constexpr float kContentHeight = 760.0f;

    ui::UIRoot* root = nullptr;
    ui::DebugOverlay* debug = nullptr;

    ui::Label* stats = nullptr;
    ui::Label* slider_value = nullptr;
    ui::Label* input_echo = nullptr;
    ui::Label* checkbox_state = nullptr;
    ui::Label* scroll_state = nullptr;
    ui::Slider* slider = nullptr;
    ui::TextInput* text_input = nullptr;
    ui::TextInput* multiline_input = nullptr;
    ui::Label* multiline_state = nullptr;
    ui::Checkbox* checkbox = nullptr;
    ui::ScrollView* scroll_vertical = nullptr;
    ui::ScrollView* scroll_both = nullptr;
    ui::Splitter* splitter = nullptr;
    ui::RadioGroup* radio_group = nullptr;
    ui::Progress* progress = nullptr;
    ui::TabBar* tab_bar = nullptr;
    ui::TabView* tab_view = nullptr;
    ui::Label* tab_view_state = nullptr;
    ui::Label* radio_state = nullptr;
    ui::Label* tab_state = nullptr;
    ui::Dropdown* dropdown = nullptr;
    ui::DialogHost* dialog_host = nullptr;
    ui::Label* dialog_state = nullptr;
    ui::MenuBar* menu_bar = nullptr;
    ui::Table* table = nullptr;
    ui::Label* table_state = nullptr;
    ui::TreeView* tree = nullptr;
    ui::Label* tree_state = nullptr;
    ui::Label* menu_state = nullptr;
    ui::UISystem* ui_system = nullptr;
    ui::Label* dropdown_state = nullptr;

    // 悬停提示目标：每个目标带自己的文本，命中第一个匹配项。
    struct TipTarget
    {
        ui::Widget* widget = nullptr;
        std::string text;
    };
    std::vector<TipTarget> tip_targets;
    ui::Widget* tip_watermark = nullptr;
    ui::Tooltip* tooltip = nullptr;
    ui::Button* theme_button = nullptr;
    ui::Button* style_button = nullptr;
    ui::Button* focus_button = nullptr;
    ui::Label* density_state = nullptr;
    ui::Label* transition_state = nullptr;
    bool light_theme = false;
    // 经典样式：直角（焦点框几何随之改为贴边框）。
    bool classic_style = false;
    // 焦点显示时机：与现代/经典几何正交，可任意组合。
    ui::FocusPolicy focus_policy = ui::FocusPolicy::Keyboard;
    // 状态过渡时长（毫秒）；0 = 关闭动画。
    float transition_ms = 120.0f;
    ui::Density density = ui::Density::Normal;

    void open_dialog()
    {
        if (dialog_host) return;
        auto dialog = std::make_unique<ui::Dialog>("Delete keyframes?", 440.0f);
        dialog->set_message(
            "This removes 3 keyframes from the selected track. Esc, the backdrop, or Cancel dismisses.");
        dialog->add_action("Cancel", [this]() {
            if (dialog_host) dialog_host->close();
        });
        dialog->add_action("Delete", [this]() {
            if (dialog_state) dialog_state->set_text("dialog: deleted");
            if (dialog_host) dialog_host->close();
        }, true);

        dialog_host = ui::show_dialog(*root, std::move(dialog), [this]() {
            dialog_host = nullptr;
            if (dialog_state) dialog_state->set_text("dialog: closed");
        });
        if (ui_system) ui_system->set_focus(dialog_host->preferred_focus());
    }
    // 主题、密度、圆角、焦点时机是四个独立维度：都从基础主题重新派生。
    void apply_theme()
    {
        ui::Theme value = light_theme ? ui::Theme::light() : ui::Theme::dark();
        root->set_theme(value.with_density(density)
                             .with_rounding(!classic_style)
                             .with_focus_policy(focus_policy)
                             .with_transition_ms(transition_ms));
        if (theme_button) {
            theme_button->set_label(light_theme ? "Theme: Light" : "Theme: Dark");
        }
        if (style_button) {
            style_button->set_label(classic_style ? "Style: Classic" : "Style: Rounded");
        }
        if (focus_button) {
            focus_button->set_label(focus_policy == ui::FocusPolicy::Keyboard ? "Focus: Keyboard"
                                    : focus_policy == ui::FocusPolicy::Always ? "Focus: Always"
                                                                             : "Focus: Never");
        }
    }
    void toggle_theme()
    {
        light_theme = !light_theme;
        apply_theme();
    }
    void set_focus_policy(ui::FocusPolicy value)
    {
        focus_policy = value;
        apply_theme();
    }
    void set_transition_ms(float value)
    {
        transition_ms = value;
        apply_theme();
        if (transition_state) {
            transition_state->set_text(value <= 0.0f ? "transition: off"
                                                     : std::format("transition: {:.0f} ms", value));
        }
    }
    void cycle_focus_policy()
    {
        switch (focus_policy) {
            case ui::FocusPolicy::Keyboard: set_focus_policy(ui::FocusPolicy::Always); break;
            case ui::FocusPolicy::Always:   set_focus_policy(ui::FocusPolicy::Never); break;
            case ui::FocusPolicy::Never:    set_focus_policy(ui::FocusPolicy::Keyboard); break;
        }
    }
    // 圆角 ↔ 直角整组切换；同时把焦点时机恢复到该风格的默认（经典 = Always），
    // 之后可用 Focus 按钮任意覆盖，两者仍然可自由组合。
    void toggle_style()
    {
        classic_style = !classic_style;
        focus_policy = classic_style ? ui::FocusPolicy::Always : ui::FocusPolicy::Keyboard;
        apply_theme();
    }
    void set_density(ui::Density value)
    {
        density = value;
        apply_theme();
        if (density_state) {
            density_state->set_text(value == ui::Density::Compact ? "density: compact"
                                    : value == ui::Density::Spacious ? "density: spacious"
                                                                     : "density: normal");
        }
    }
    std::vector<ui::Button*> nav_buttons;
    std::vector<ui::ScrollPanel*> sections;
    std::vector<std::string> section_names;

    void show_section(std::size_t index)
    {
        for (std::size_t i = 0; i < sections.size(); ++i) {
            sections[i]->set_visible(i == index);
        }
        for (std::size_t i = 0; i < nav_buttons.size(); ++i) {
            nav_buttons[i]->set_label(i == index
                ? "> " + section_names[i]
                : "  " + section_names[i]);
        }
    }

    void update_tooltip(glm::vec2 cursor, float dt)
    {
        const std::string* text = nullptr;
        for (const auto& target : tip_targets) {
            if (!target.widget || !target.widget->visible()) continue;
            if (!target.widget->global_bounds().contains(cursor)) continue;
            text = &target.text;
            break;
        }

        if (text) tooltip->set_text(*text);

        // 悬停延迟、跟随光标与边界翻转都由 Tooltip 自己驱动。
        const ui::Rect bounds = root ? root->global_bounds()
                                     : ui::Rect{glm::vec2{0.0f, 0.0f}, glm::vec2{0.0f, 0.0f}};
        tooltip->update_hover(dt, text != nullptr, cursor, bounds);
    }
};

namespace detail
{
inline std::unique_ptr<ui::ScrollPanel> make_section(const std::string& title)
{
    auto panel = std::make_unique<ui::ScrollPanel>(UiGallery::kContentX, 16.0f,
                                                  UiGallery::kContentWidth, UiGallery::kContentHeight);
    panel->set_min_size({UiGallery::kContentWidth, UiGallery::kContentHeight});
    panel->set_max_size({UiGallery::kContentWidth, UiGallery::kContentHeight});
    panel->content()->set_cross_align(ui::Align::top_left());
    panel->add(std::make_unique<ui::Label>(title, 22.0f));
    return panel;
}

inline std::unique_ptr<ui::ScrollView> make_scroll(float width, float height)
{
    auto scroll = std::make_unique<ui::ScrollView>();
    scroll->set_min_size({width, height});
    scroll->set_max_size({width, height});
    return scroll;
}
} // namespace detail

inline UiGallery build(ui::UIRoot& root)
{
    UiGallery gallery;
    gallery.root = &root;
    gallery.section_names = {"Overview", "Text", "Inputs", "Containers", "Overlays", "Menu", "Layout", "Style"};

    // ---------- 左侧导航 ----------
    auto nav = std::make_unique<ui::Panel>(16.0f, 16.0f, UiGallery::kNavWidth, UiGallery::kContentHeight);
    nav->set_min_size({UiGallery::kNavWidth, UiGallery::kContentHeight});
    nav->set_max_size({UiGallery::kNavWidth, UiGallery::kContentHeight});
    nav->set_cross_align(ui::Align::top_left());
    nav->add(std::make_unique<ui::Label>("UI Gallery", 20.0f));
    nav->add(std::make_unique<ui::Label>("Tab to navigate", 12.0f));

    for (std::size_t i = 0; i < gallery.section_names.size(); ++i) {
        auto button = std::make_unique<ui::Button>(gallery.section_names[i], [&gallery, i]() {
            gallery.show_section(i);
        });
        gallery.nav_buttons.push_back(button.get());
        nav->add(std::move(button));
    }

    nav->add(std::make_unique<ui::Spacer>(1.0f, 12.0f));
    auto theme_button = std::make_unique<ui::Button>("Theme: Dark", [&gallery]() {
        gallery.toggle_theme();
    });
    gallery.theme_button = theme_button.get();
    nav->add(std::move(theme_button));
    auto style_button = std::make_unique<ui::Button>("Style: Rounded", [&gallery]() {
        gallery.toggle_style();
    });
    gallery.style_button = style_button.get();
    nav->add(std::move(style_button));
    auto focus_button = std::make_unique<ui::Button>("Focus: Keyboard", [&gallery]() {
        gallery.cycle_focus_policy();
    });
    gallery.focus_button = focus_button.get();
    nav->add(std::move(focus_button));
    nav->add(std::make_unique<ui::Button>("Toggle debug (F1)", [&gallery]() {
        gallery.debug->toggle();
    }));
    nav->add(std::make_unique<ui::Label>("v" + std::string(xel::version_string()), 12.0f));
    root.add(std::move(nav));

    // ---------- Overview ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Overview")};
        section->add(std::make_unique<ui::Label>(
            "All controls below are retained-mode widgets: layout, events, text and clipping live in the UI layer.",
            14.0f));
        auto stats = std::make_unique<ui::Label>("frame: -");
        gallery.stats = stats.get();
        section->add(std::move(stats));
        section->add(std::make_unique<ui::Label>(
            "Sections: Text, Inputs, Containers, Overlays, Layout.\n"
            "Press Tab / Shift+Tab to walk the focus order; Enter or Space activates the focused control.",
            14.0f));
        auto watermark = std::make_unique<ui::Label>("Hover this line to see an overlay tooltip.", 16.0f,
                                                     glm::vec4{0.35f, 0.72f, 1.0f, 1.0f});
        gallery.tip_watermark = watermark.get();
        gallery.tip_targets.push_back({watermark.get(), "Overlay layer stays above panels and clips"});
        section->add(std::move(watermark));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Text ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Text")};
        section->add(std::make_unique<ui::Label>("Text sizes and colors", 18.0f));
        section->add(std::make_unique<ui::Label>("12 px label", 12.0f));
        section->add(std::make_unique<ui::Label>("16 px label (theme default)", 16.0f));
        section->add(std::make_unique<ui::Label>("24 px accent label", 24.0f,
                                                 glm::vec4{0.35f, 0.72f, 1.0f, 1.0f}));
        auto long_label = std::make_unique<ui::Label>(
            "Wrapped text with ellipsis: this sentence is intentionally long so that it wraps across the "
            "available width and ends with dots when it exceeds the configured line limit.", 14.0f);
        long_label->set_wrap(true);
        long_label->set_max_size({820.0f, 60.0f});
        section->add(std::move(long_label));
        auto wrapped = std::make_unique<ui::Label>(
            "Wrapped + 2 line limit: the quick brown fox jumps over the lazy dog, again and again and again, "
            "until the label is forced to truncate with an ellipsis.");
        wrapped->set_wrap(true);
        wrapped->set_max_lines(2);
        wrapped->set_ellipsis(true);
        wrapped->set_max_size({820.0f, 60.0f});
        section->add(std::move(wrapped));
        section->add(std::make_unique<ui::Label>("中文：你好，世界。UTF-8 + GB2312 一级常用字。", 16.0f));
        section->add(std::make_unique<ui::Label>("动态字形：囧、犇、龘（首次绘制时加入图集）", 16.0f));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Inputs ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Inputs")};
        section->add(std::make_unique<ui::Label>("Buttons", 18.0f));
        auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 700.0f, 40.0f);
        auto normal = std::make_unique<ui::Button>("Enabled", [] {});
        gallery.tip_targets.push_back({normal.get(), "Focused widgets can be activated with Enter or Space"});
        row->add(std::move(normal));
        auto disabled = std::make_unique<ui::Button>("Disabled");
        disabled->set_enabled(false);
        row->add(std::move(disabled));
        section->add(std::move(row));

        section->add(std::make_unique<ui::Label>("Checkbox", 18.0f));
        auto checkbox = std::make_unique<ui::Checkbox>("Enable feature", true, [&gallery](bool checked) {
            gallery.checkbox_state->set_text(checked ? "checkbox: checked" : "checkbox: unchecked");
        });
        gallery.checkbox = checkbox.get();
        section->add(std::move(checkbox));
        auto checkbox_state = std::make_unique<ui::Label>("checkbox: checked", 14.0f);
        gallery.checkbox_state = checkbox_state.get();
        section->add(std::move(checkbox_state));

        section->add(std::make_unique<ui::Label>("Slider", 18.0f));
        auto slider = std::make_unique<ui::Slider>(480.0f);
        slider->set_range(0.0f, 100.0f);
        slider->set_step(1.0f);
        slider->set_value(40.0f);
        gallery.slider = slider.get();
        slider->set_on_change([&gallery](float value) {
            gallery.slider_value->set_text(std::format("slider: {:.0f}", value));
            if (gallery.progress) gallery.progress->set_value(value / 100.0f);
        });
        section->add(std::move(slider));
        auto slider_value = std::make_unique<ui::Label>("slider: 40", 14.0f);
        gallery.slider_value = slider_value.get();
        section->add(std::move(slider_value));

        section->add(std::make_unique<ui::Label>("Text input (UTF-8, Ctrl+A, selection)", 18.0f));
        auto input = std::make_unique<ui::TextInput>("hello", 480.0f);
        gallery.text_input = input.get();
        input->set_on_change([&gallery](const std::string& value) {
            gallery.input_echo->set_text("input: " + value);
        });
        section->add(std::move(input));
        auto echo = std::make_unique<ui::Label>("input: hello", 14.0f);
        gallery.input_echo = echo.get();
        section->add(std::move(echo));

        section->add(std::make_unique<ui::Label>("Multiline input (Enter = newline, Up/Down)", 18.0f));
        auto multiline = std::make_unique<ui::TextInput>("line one\nline two\nline three", 480.0f);
        multiline->set_multiline(true);
        multiline->set_preferred_size({480.0f, 108.0f});
        gallery.multiline_input = multiline.get();
        multiline->set_on_change([&gallery](const std::string& value) {
            const std::size_t lines = 1 + static_cast<std::size_t>(
                std::count(value.begin(), value.end(), '\n'));
            gallery.multiline_state->set_text(std::format(
                "multiline: {} chars, {} lines", value.size(), lines));
        });
        section->add(std::move(multiline));
        auto multiline_state = std::make_unique<ui::Label>("multiline: 28 chars, 3 lines", 14.0f);
        gallery.multiline_state = multiline_state.get();
        section->add(std::move(multiline_state));
        section->add(std::make_unique<ui::Label>("Radio group (single selection)", 18.0f));
        auto radios = std::make_unique<ui::RadioGroup>(300.0f);
        gallery.radio_group = radios.get();
        radios->set_options({"Small", "Medium", "Large"}, 1);
        radios->set_on_change([&gallery](int index) {
            gallery.radio_state->set_text("radio: " + gallery.radio_group->option(index)->label());
        });
        section->add(std::move(radios));
        auto radio_state = std::make_unique<ui::Label>("radio: Medium", 14.0f);
        gallery.radio_state = radio_state.get();
        section->add(std::move(radio_state));

        section->add(std::make_unique<ui::Label>("Progress (follows slider)", 18.0f));
        auto progress = std::make_unique<ui::Progress>(480.0f);
        gallery.progress = progress.get();
        progress->set_value(0.4f);
        section->add(std::move(progress));

        section->add(std::make_unique<ui::Label>("Dropdown (overlay list, arrow keys)", 18.0f));
        auto dropdown = std::make_unique<ui::Dropdown>(
            std::vector<std::string>{"Solid", "Dashed", "Dotted"}, 0);
        gallery.dropdown = dropdown.get();
        dropdown->set_on_change([&gallery](int index) {
            gallery.dropdown_state->set_text("dropdown: " + gallery.dropdown->selected_text());
        });
        section->add(std::move(dropdown));
        auto dropdown_state = std::make_unique<ui::Label>("dropdown: Solid", 14.0f);
        gallery.dropdown_state = dropdown_state.get();
        section->add(std::move(dropdown_state));
        section->add(std::make_unique<ui::Label>("TabBar (click or Left/Right when focused)", 18.0f));
        auto tabs = std::make_unique<ui::TabBar>();
        gallery.tab_bar = tabs.get();
        tabs->set_tabs({"Overview", "Stats", "Preview"});
        tabs->set_on_change([&gallery](int index) {
            gallery.tab_state->set_text("tab: " + gallery.tab_bar->tabs()[static_cast<std::size_t>(index)]);
        });
        section->add(std::move(tabs));
        auto tab_state = std::make_unique<ui::Label>("tab: Overview", 14.0f);
        gallery.tab_state = tab_state.get();
        section->add(std::move(tab_state));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Containers ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Containers")};
        section->add(std::make_unique<ui::Label>("Splitter (drag the middle bar)", 18.0f));
        auto splitter = std::make_unique<ui::Splitter>(620.0f, 140.0f);
        splitter->set_min_size({620.0f, 140.0f});
        splitter->set_max_size({620.0f, 140.0f});
        splitter->set_panes(
            std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f, glm::vec4{0.18f, 0.42f, 0.62f, 1.0f}),
            std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f, glm::vec4{0.24f, 0.58f, 0.38f, 1.0f}));
        gallery.splitter = splitter.get();
        section->add(std::move(splitter));

        section->add(std::make_unique<ui::Label>("TabView (TabBar + page container)", 18.0f));
        auto tab_view = std::make_unique<ui::TabView>();
        tab_view->set_min_size({620.0f, 180.0f});
        tab_view->set_max_size({620.0f, 180.0f});
        {
            auto alpha = std::make_unique<ui::Vbox>(0.0f, 0.0f, 600.0f, 120.0f);
            alpha->set_cross_align(ui::Align::top_left());
            alpha->add(std::make_unique<ui::Label>("Alpha page: only the selected page is hit-testable."));
            alpha->add(std::make_unique<ui::Button>("Alpha button", [] {}));
            tab_view->add_page("Alpha", std::move(alpha));

            auto beta = std::make_unique<ui::Vbox>(0.0f, 0.0f, 600.0f, 120.0f);
            beta->set_cross_align(ui::Align::top_left());
            beta->add(std::make_unique<ui::Label>("Beta page: switch with click or Left/Right."));
            auto beta_progress = std::make_unique<ui::Progress>(320.0f);
            beta_progress->set_value(0.45f);
            beta->add(std::move(beta_progress));
            tab_view->add_page("Beta", std::move(beta));

            auto gamma = std::make_unique<ui::Vbox>(0.0f, 0.0f, 600.0f, 120.0f);
            gamma->set_cross_align(ui::Align::top_left());
            gamma->add(std::make_unique<ui::Label>("Gamma page: pages are laid out eagerly."));
            gamma->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 320.0f, 24.0f,
                                                       glm::vec4{0.24f, 0.58f, 0.38f, 1.0f}));
            tab_view->add_page("Gamma", std::move(gamma));
        }
        auto tab_view_state = std::make_unique<ui::Label>("tab view: Alpha", 12.0f);
        gallery.tab_view_state = tab_view_state.get();
        tab_view->set_on_change([&gallery](int index) {
            gallery.tab_view_state->set_text("tab view: " + gallery.tab_view->tab_bar().tabs()[static_cast<std::size_t>(index)]);
        });
        gallery.tab_view = tab_view.get();
        section->add(std::move(tab_view));
        section->add(std::move(tab_view_state));

        section->add(std::make_unique<ui::Label>("Table (header, selection, wheel scroll)", 18.0f));
        auto table = std::make_unique<ui::Table>(620.0f, 210.0f);
        table->set_columns({
            {"Asset", 240.0f, ui::TableColumn::Align::Left},
            {"Type", 130.0f, ui::TableColumn::Align::Left},
            {"Size", 0.0f, ui::TableColumn::Align::Right},
        });
        std::vector<std::vector<std::string>> table_rows;
        const char* kinds[] = {"image", "audio", "script", "model"};
        for (int i = 0; i < 14; ++i) {
            table_rows.push_back({
                std::format("asset_{:02d}.png", i),
                kinds[i % 4],
                std::format("{} KB", 12 + i * 7),
            });
        }
        table->set_rows(table_rows);
        table->set_selected(0);
        auto table_state = std::make_unique<ui::Label>("table: asset_00.png", 12.0f);
        gallery.table_state = table_state.get();
        table->set_on_select([&gallery](int index) {
            if (!gallery.table_state || !gallery.table) return;
            const auto& rows = gallery.table->rows();
            if (index < 0 || index >= static_cast<int>(rows.size())) return;
            gallery.table_state->set_text("table: " + rows[static_cast<std::size_t>(index)][0]);
        });
        gallery.table = table.get();
        section->add(std::move(table));
        section->add(std::move(table_state));

        section->add(std::make_unique<ui::Label>("TreeView (indent, expand/collapse, keyboard)", 18.0f));
        section->add(std::make_unique<ui::Label>(
            "Click a chevron to expand or collapse; Up/Down/Home/End move, Right/Left expand/collapse or step to parent, Enter/Space toggles.",
            14.0f));
        auto tree = std::make_unique<ui::TreeView>(620.0f, 220.0f);
        {
            auto leaf = [](std::string title) {
                ui::TreeNode node;
                node.title = std::move(title);
                return node;
            };
            auto branch = [](std::string title, std::vector<ui::TreeNode> children, bool expanded) {
                ui::TreeNode node;
                node.title = std::move(title);
                node.children = std::move(children);
                node.expanded = expanded;
                return node;
            };

            std::vector<ui::TreeNode> tree_roots;
            tree_roots.push_back(branch("src", {
                leaf("xel_application.cpp"),
                branch("ui", {leaf("widget.hpp"), leaf("tree_view.hpp"), leaf("table.hpp")}, true),
                branch("backend", {
                    leaf("window.cpp"),
                    branch("vulkan", {leaf("renderer.cpp"), leaf("swap_chain.cpp")}, false),
                }, false),
            }, true));
            tree_roots.push_back(branch("assets", {
                branch("fonts", {leaf("default8.png"), leaf("cjk.ttf")}, true),
                leaf("shaders"),
            }, false));
            tree_roots.push_back(leaf("README.md"));
            tree->set_roots(std::move(tree_roots));
            tree->set_selected_path({0, 1, 1});
        }
        auto tree_state = std::make_unique<ui::Label>("tree: src/ui/tree_view.hpp", 12.0f);
        gallery.tree_state = tree_state.get();
        tree->set_on_select([&gallery](const std::vector<int>& path) {
            if (!gallery.tree_state || !gallery.tree) return;

            std::string joined;
            for (std::size_t i = 0; i < path.size(); ++i) {
                std::vector<int> prefix(path.begin(),
                    path.begin() + static_cast<std::vector<int>::difference_type>(i + 1));
                auto* node = gallery.tree->node_at(prefix);
                if (!node) continue;
                if (!joined.empty()) joined += '/';
                joined += node->title;
            }
            if (!joined.empty()) gallery.tree_state->set_text("tree: " + joined);
        });
        gallery.tree = tree.get();
        section->add(std::move(tree));
        section->add(std::move(tree_state));

        section->add(std::make_unique<ui::Label>("ScrollView (vertical, clips content)", 18.0f));
        auto scroll = detail::make_scroll(620.0f, 160.0f);
        auto content = std::make_unique<ui::Vbox>(0.0f, 0.0f, 600.0f, 0.0f);
        for (int i = 0; i < 12; ++i) {
            content->add(std::make_unique<ui::ColorRect>(
                0.0f, 0.0f, 600.0f, 34.0f,
                glm::vec4{0.20f + 0.02f * static_cast<float>(i), 0.30f, 0.45f, 1.0f}));
        }
        scroll->set_content(std::move(content));
        gallery.scroll_vertical = scroll.get();
        section->add(std::move(scroll));

        section->add(std::make_unique<ui::Label>("ScrollView (both axes)", 18.0f));
        auto both = detail::make_scroll(620.0f, 150.0f);
        both->set_axis(ui::ScrollView::Axis::Both);
        auto grid = std::make_unique<ui::Vbox>(0.0f, 0.0f, 1000.0f, 0.0f);
        for (int row_index = 0; row_index < 8; ++row_index) {
            auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 1000.0f, 34.0f);
            for (int col = 0; col < 10; ++col) {
                const float t = static_cast<float>(row_index * 10 + col) / 80.0f;
                row->add(std::make_unique<ui::ColorRect>(
                    0.0f, 0.0f, 96.0f, 34.0f,
                    glm::vec4{0.15f + t * 0.5f, 0.35f, 0.65f - t * 0.3f, 1.0f}));
            }
            grid->add(std::move(row));
        }
        both->set_content(std::move(grid));
        gallery.scroll_both = both.get();
        auto scroll_state = std::make_unique<ui::Label>("scroll: 0, 0", 12.0f);
        gallery.scroll_state = scroll_state.get();
        section->add(std::move(both));
        section->add(std::move(scroll_state));

        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Overlays ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Overlays")};
        section->add(std::make_unique<ui::Label>(
            "Overlays are drawn after normal children and receive hit tests first.\n"
            "Hover a target: the tooltip waits 350 ms, follows the cursor, and flips near window edges.",
            14.0f));
        auto button = std::make_unique<ui::Button>("Hover for tooltip", [] {});
        gallery.tip_targets.push_back({button.get(), "Tooltips appear after a 350 ms hover delay"});
        section->add(std::move(button));
        section->add(std::make_unique<ui::Label>(
            "Hover the wide bar below: near the right edge the tooltip flips to the upper-left.",
            14.0f));
        auto edge_target = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 940.0f, 40.0f,
                                                           glm::vec4{0.24f, 0.42f, 0.72f, 1.0f});
        gallery.tip_targets.push_back({edge_target.get(),
            "Near the edge this tooltip flips to the upper-left and stays inside the window"});
        section->add(std::move(edge_target));
        section->add(std::make_unique<ui::Label>(
            "Dialog / Modal: centered card over a scrim, Esc or backdrop click dismisses.",
            14.0f));
        auto dialog_state = std::make_unique<ui::Label>("dialog: closed", 12.0f);
        gallery.dialog_state = dialog_state.get();
        section->add(std::make_unique<ui::Button>("Open dialog", [&gallery]() {
            gallery.open_dialog();
        }));
        section->add(std::move(dialog_state));

        section->add(std::make_unique<ui::Label>(
            "The debug overlay (F1) is also an overlay: it shows frame time, draw calls, vertices and indices.",
            14.0f));
        section->add(std::make_unique<ui::Button>("Toggle debug overlay", [&gallery]() {
            gallery.debug->toggle();
        }));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Menu ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Menu")};
        section->add(std::make_unique<ui::Label>(
            "MenuBar: click a title to open, hover titles to switch, Esc or the backdrop closes.",
            14.0f));

        auto menu = std::make_unique<ui::MenuBar>();
        menu->add_menu("File", {
            ui::MenuItem::make("New", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: File > New");
            }, "Ctrl+N"),
            ui::MenuItem::make("Open", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: File > Open");
            }, "Ctrl+O"),
            ui::MenuItem::separator_item(),
            ui::MenuItem::make("Save", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: File > Save");
            }, "Ctrl+S"),
            ui::MenuItem::make("Export (disabled)", [] {}, "Ctrl+E", false),
        });
        menu->add_menu("Edit", {
            ui::MenuItem::make("Undo", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: Edit > Undo");
            }, "Ctrl+Z"),
            ui::MenuItem::make("Redo", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: Edit > Redo");
            }, "Ctrl+Y"),
            ui::MenuItem::separator_item(),
            ui::MenuItem::make("Select all", [&gallery]() {
                if (gallery.menu_state) gallery.menu_state->set_text("menu: Edit > Select all");
            }, "Ctrl+A"),
        });
        menu->add_menu("View", {
            ui::MenuItem::make("Toggle debug overlay", [&gallery]() {
                if (gallery.debug) gallery.debug->toggle();
                if (gallery.menu_state) gallery.menu_state->set_text("menu: View > Debug overlay");
            }, "F1"),
            ui::MenuItem::make("Toggle theme", [&gallery]() {
                gallery.toggle_theme();
                if (gallery.menu_state) gallery.menu_state->set_text("menu: View > Theme");
            }),
        });
        gallery.menu_bar = menu.get();
        section->add(std::move(menu));

        auto menu_state = std::make_unique<ui::Label>("menu: none", 12.0f);
        gallery.menu_state = menu_state.get();
        section->add(std::move(menu_state));

        section->add(std::make_unique<ui::Label>(
            "Keyboard: focus the bar, Down/Enter opens, Up/Down moves, Left/Right switches menus, Esc closes.",
            14.0f));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Layout ----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Layout")};

        section->add(std::make_unique<ui::Label>("Vbox main align: Start / Center / End / SpaceBetween", 18.0f));
        auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 900.0f, 200.0f);
        row->set_min_size({900.0f, 200.0f});
        row->set_max_size({900.0f, 200.0f});
        row->set_cross_align(ui::Align::v_stretch());
        const ui::MainAlign aligns[] = {ui::MainAlign::Start, ui::MainAlign::Center,
                                        ui::MainAlign::End, ui::MainAlign::SpaceBetween};
        for (auto align : aligns) {
            auto column = std::make_unique<ui::Vbox>(0.0f, 0.0f, 190.0f, 200.0f);
            column->set_min_size({190.0f, 200.0f});
            column->set_max_size({190.0f, 200.0f});
            column->set_align(ui::Align::v_stretch());
            column->set_main_align(align);
            column->set_spacing(6.0f);
            column->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 60.0f, 24.0f, glm::vec4{0.35f, 0.55f, 0.85f, 1.0f}));
            column->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 60.0f, 24.0f, glm::vec4{0.45f, 0.65f, 0.90f, 1.0f}));
            column->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 60.0f, 24.0f, glm::vec4{0.55f, 0.75f, 0.95f, 1.0f}));
            row->add(std::move(column));
        }
        section->add(std::move(row));

        section->add(std::make_unique<ui::Label>("Hbox cross align: Top / Center / Bottom", 18.0f));
        const ui::Align cross_aligns[] = {ui::Align::top_left(), ui::Align::center_left(),
                                          ui::Align::bottom_left()};
        for (auto align : cross_aligns) {
            auto cross_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 620.0f, 70.0f);
            cross_row->set_min_size({620.0f, 70.0f});
            cross_row->set_max_size({620.0f, 70.0f});
            cross_row->set_cross_align(ui::Align::v_stretch());
            auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 80.0f, 28.0f, glm::vec4{0.45f, 0.62f, 0.35f, 1.0f});
            rect->set_align(align);
            cross_row->add(std::move(rect));
            section->add(std::move(cross_row));
        }

        section->add(std::make_unique<ui::Label>("flex_grow: 1 / 2 / 3", 18.0f));
        auto grow_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 620.0f, 48.0f);
        grow_row->set_min_size({620.0f, 48.0f});
        grow_row->set_max_size({620.0f, 48.0f});
        for (int i = 1; i <= 3; ++i) {
            auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 60.0f, 48.0f,
                                                        glm::vec4{0.30f, 0.30f + 0.12f * static_cast<float>(i), 0.75f, 1.0f});
            rect->set_flex_grow(static_cast<float>(i));
            grow_row->add(std::move(rect));
        }
        section->add(std::move(grow_row));

        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }
    // ---------- Style：样式系统（角色 / 状态 / 圆角 / 密度）----------
    {
        std::unique_ptr<ui::ScrollPanel> section{detail::make_section("Style")};
        section->add(std::make_unique<ui::Label>(
            "Each control resolves a BoxStyle from Theme by role + state: Surface, Control, Input, Track, "
            "Accent, Row, MenuItem, Tooltip. Fill, border, radius, accent and density all come from the theme.",
            14.0f));

        section->add(std::make_unique<ui::Label>("Rounded vs classic preset", 18.0f));
        auto presets = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        presets->set_spacing(8.0f);
        presets->add(std::make_unique<ui::Button>("Rounded", [&gallery]() {
            gallery.classic_style = false;
            gallery.apply_theme();
        }));
        presets->add(std::make_unique<ui::Button>("Classic", [&gallery]() {
            gallery.classic_style = true;
            gallery.apply_theme();
        }));
        section->add(std::move(presets));
        section->add(std::make_unique<ui::Label>(
            "Classic keeps the pre-style-system look: square corners and a focus border drawn inside the widget. "
            "Rounded adds corner radius and an outline ring outside the widget.",
            14.0f));

        section->add(std::make_unique<ui::Label>("Focus reveal (independent of corners)", 18.0f));
        auto policies = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        policies->set_spacing(8.0f);
        policies->add(std::make_unique<ui::Button>("Keyboard only", [&gallery]() {
            gallery.set_focus_policy(ui::FocusPolicy::Keyboard);
        }));
        policies->add(std::make_unique<ui::Button>("Always", [&gallery]() {
            gallery.set_focus_policy(ui::FocusPolicy::Always);
        }));
        policies->add(std::make_unique<ui::Button>("Never", [&gallery]() {
            gallery.set_focus_policy(ui::FocusPolicy::Never);
        }));
        section->add(std::move(policies));
        section->add(std::make_unique<ui::Label>(
            "Keyboard only paints while the keyboard is the active input; Always paints on focus; Never hides it. "
            "All three work with both Rounded and Classic corners.",
            14.0f));

        section->add(std::make_unique<ui::Label>("Control states", 18.0f));
        auto states = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        states->set_spacing(8.0f);
        states->add(std::make_unique<ui::Button>("Normal"));
        auto hover_state = std::make_unique<ui::Button>("Hover");
        hover_state->set_hovered(true);
        states->add(std::move(hover_state));
        auto pressed_state = std::make_unique<ui::Button>("Pressed");
        pressed_state->set_pressed(true);
        states->add(std::move(pressed_state));
        auto disabled_state = std::make_unique<ui::Button>("Disabled");
        disabled_state->set_enabled(false);
        states->add(std::move(disabled_state));
        auto focus_state = std::make_unique<ui::Button>("Keyboard focus");
        focus_state->set_focused(true);
        states->add(std::move(focus_state));
        section->add(std::move(states));
        section->add(std::make_unique<ui::Label>(
            "The focus ring paints only while the keyboard is the active input source: press Tab to reveal it, "
            "then move the mouse to hide it.", 14.0f));

        section->add(std::make_unique<ui::Label>("Corner radius (per-widget override)", 18.0f));
        auto radii = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        radii->set_spacing(8.0f);
        for (float radius : {0.0f, 4.0f, 10.0f, 18.0f}) {
            auto button = std::make_unique<ui::Button>(std::format("radius {:.0f}", radius));
            button->set_corner_radius(radius);
            radii->add(std::move(button));
        }
        section->add(std::move(radii));

        section->add(std::make_unique<ui::Label>("Density", 18.0f));
        auto densities = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        densities->set_spacing(8.0f);
        densities->add(std::make_unique<ui::Button>("Compact", [&gallery]() {
            gallery.set_density(ui::Density::Compact);
        }));
        densities->add(std::make_unique<ui::Button>("Normal", [&gallery]() {
            gallery.set_density(ui::Density::Normal);
        }));
        densities->add(std::make_unique<ui::Button>("Spacious", [&gallery]() {
            gallery.set_density(ui::Density::Spacious);
        }));
        section->add(std::move(densities));
        auto density_label = std::make_unique<ui::Label>("density: normal", 14.0f);
        gallery.density_state = density_label.get();
        section->add(std::move(density_label));

        section->add(std::make_unique<ui::Label>("State transition", 18.0f));
        auto transitions = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        transitions->set_spacing(8.0f);
        transitions->add(std::make_unique<ui::Button>("Off", [&gallery]() {
            gallery.set_transition_ms(0.0f);
        }));
        transitions->add(std::make_unique<ui::Button>("60 ms", [&gallery]() {
            gallery.set_transition_ms(60.0f);
        }));
        transitions->add(std::make_unique<ui::Button>("120 ms", [&gallery]() {
            gallery.set_transition_ms(120.0f);
        }));
        transitions->add(std::make_unique<ui::Button>("240 ms", [&gallery]() {
            gallery.set_transition_ms(240.0f);
        }));
        section->add(std::move(transitions));
        auto transition_label = std::make_unique<ui::Label>("transition: 120 ms", 14.0f);
        gallery.transition_state = transition_label.get();
        section->add(std::move(transition_label));

        section->add(std::make_unique<ui::Label>("Accent (checkbox / radio / slider / progress)", 18.0f));
        auto accent_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 760.0f, 40.0f);
        accent_row->set_spacing(12.0f);
        accent_row->add(std::make_unique<ui::Checkbox>("checked", true));
        accent_row->add(std::make_unique<ui::Radio>("selected", true));
        auto accent_slider = std::make_unique<ui::Slider>(220.0f);
        accent_slider->set_value(0.65f);
        accent_row->add(std::move(accent_slider));
        section->add(std::move(accent_row));

        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Overlay 层 ----------
    auto tooltip = std::make_unique<ui::Tooltip>("");
    gallery.tooltip = tooltip.get();
    root.add_overlay(std::move(tooltip));

    auto debug = std::make_unique<ui::DebugOverlay>();
    gallery.debug = debug.get();
    root.add_overlay(std::move(debug));

    gallery.show_section(0);
    if (const char* theme = std::getenv("COSINE_GALLERY_THEME"); theme && std::strcmp(theme, "light") == 0) {
        gallery.toggle_theme();
    }
    if (std::getenv("COSINE_GALLERY_CLASSIC")) {
        gallery.classic_style = true;
        gallery.apply_theme();
    }
    if (const char* focus = std::getenv("COSINE_GALLERY_FOCUS")) {
        if (std::strcmp(focus, "always") == 0) {
            gallery.set_focus_policy(ui::FocusPolicy::Always);
        } else if (std::strcmp(focus, "never") == 0) {
            gallery.set_focus_policy(ui::FocusPolicy::Never);
        } else if (std::strcmp(focus, "keyboard") == 0) {
            gallery.set_focus_policy(ui::FocusPolicy::Keyboard);
        }
    }
    if (const char* transition = std::getenv("COSINE_GALLERY_TRANSITION")) {
        if (std::strcmp(transition, "off") == 0) {
            gallery.set_transition_ms(0.0f);
        } else {
            gallery.set_transition_ms(static_cast<float>(std::atof(transition)));
        }
    }
    if (const char* section = std::getenv("COSINE_GALLERY_SECTION")) {
        const int index = std::atoi(section);
        if (index >= 0 && index < static_cast<int>(gallery.sections.size())) {
            gallery.show_section(static_cast<std::size_t>(index));
        }
    }
    if (std::getenv("COSINE_GALLERY_DIALOG")) gallery.open_dialog();
    if (const char* menu = std::getenv("COSINE_GALLERY_MENU"); menu && gallery.menu_bar) {
        gallery.menu_bar->open(std::atoi(menu));
    }
    return gallery;
}
} // namespace xel::demo
