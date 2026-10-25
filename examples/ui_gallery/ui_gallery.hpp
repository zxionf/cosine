#pragma once

#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/color_rect.hpp"
#include "ui/dropdown.hpp"
#include "ui/debug_overlay.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/list.hpp"
#include "ui/panel.hpp"
#include "ui/progress.hpp"
#include "ui/radio_group.hpp"
#include "ui/root.hpp"
#include "ui/scroll_view.hpp"
#include "ui/slider.hpp"
#include "ui/tab_bar.hpp"
#include "ui/spacer.hpp"
#include "ui/splitter.hpp"
#include "ui/text_input.hpp"
#include "ui/tooltip.hpp"
#include "ui/vbox.hpp"
#include "xel/version.hpp"

#include <cstdlib>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <vector>

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
    ui::Checkbox* checkbox = nullptr;
    ui::ScrollView* scroll_vertical = nullptr;
    ui::ScrollView* scroll_both = nullptr;
    ui::Splitter* splitter = nullptr;
    ui::RadioGroup* radio_group = nullptr;
    ui::Progress* progress = nullptr;
    ui::TabBar* tab_bar = nullptr;
    ui::Label* radio_state = nullptr;
    ui::Label* tab_state = nullptr;
    ui::Dropdown* dropdown = nullptr;
    ui::Label* dropdown_state = nullptr;

    ui::Widget* tip_button = nullptr;
    ui::Widget* tip_watermark = nullptr;
    ui::Tooltip* tooltip = nullptr;
    ui::Button* theme_button = nullptr;
    bool light_theme = false;

    void toggle_theme()
    {
        light_theme = !light_theme;
        root->set_theme(light_theme ? ui::Theme::light() : ui::Theme::dark());
        if (theme_button) {
            theme_button->set_label(light_theme ? "Theme: Light" : "Theme: Dark");
        }
    }
    std::vector<ui::Button*> nav_buttons;
    std::vector<ui::Panel*> sections;
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

    void update_tooltip(glm::vec2 cursor)
    {
        const ui::Widget* target = nullptr;
        if (tip_button && tip_button->visible() && tip_button->global_bounds().contains(cursor)) {
            target = tip_button;
        } else if (tip_watermark && tip_watermark->visible() && tip_watermark->global_bounds().contains(cursor)) {
            target = tip_watermark;
        }

        if (!target) {
            tooltip->hide();
            return;
        }

        if (target == tip_button) {
            tooltip->set_text("Focused widgets can be activated with Enter or Space");
        } else {
            tooltip->set_text("Overlay layer stays above panels and clips");
        }
        tooltip->show_at({cursor.x + 16.0f, cursor.y + 18.0f});
    }
};

namespace detail
{
inline ui::Panel* make_section(const std::string& title)
{
    auto panel = std::make_unique<ui::Panel>(UiGallery::kContentX, 16.0f,
                                            UiGallery::kContentWidth, UiGallery::kContentHeight);
    panel->set_min_size({UiGallery::kContentWidth, UiGallery::kContentHeight});
    panel->set_max_size({UiGallery::kContentWidth, UiGallery::kContentHeight});
    panel->set_cross_align(ui::Align::top_left());
    panel->add(std::make_unique<ui::Label>(title, 22.0f));
    return panel.release();
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
    gallery.section_names = {"Overview", "Text", "Inputs", "Containers", "Overlays", "Layout"};

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
    nav->add(std::make_unique<ui::Button>("Toggle debug (F1)", [&gallery]() {
        gallery.debug->toggle();
    }));
    nav->add(std::make_unique<ui::Label>("v" + std::string(xel::version_string()), 12.0f));
    root.add(std::move(nav));

    // ---------- Overview ----------
    {
        std::unique_ptr<ui::Panel> section{detail::make_section("Overview")};
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
        section->add(std::move(watermark));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Text ----------
    {
        std::unique_ptr<ui::Panel> section{detail::make_section("Text")};
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
        std::unique_ptr<ui::Panel> section{detail::make_section("Inputs")};
        section->add(std::make_unique<ui::Label>("Buttons", 18.0f));
        auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 700.0f, 40.0f);
        auto normal = std::make_unique<ui::Button>("Enabled", [] {});
        gallery.tip_button = normal.get();
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
        std::unique_ptr<ui::Panel> section{detail::make_section("Containers")};
        section->add(std::make_unique<ui::Label>("Splitter (drag the middle bar)", 18.0f));
        auto splitter = std::make_unique<ui::Splitter>(620.0f, 140.0f);
        splitter->set_min_size({620.0f, 140.0f});
        splitter->set_max_size({620.0f, 140.0f});
        splitter->set_panes(
            std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f, glm::vec4{0.18f, 0.42f, 0.62f, 1.0f}),
            std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f, glm::vec4{0.24f, 0.58f, 0.38f, 1.0f}));
        gallery.splitter = splitter.get();
        section->add(std::move(splitter));

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
        std::unique_ptr<ui::Panel> section{detail::make_section("Overlays")};
        section->add(std::make_unique<ui::Label>(
            "Overlays are drawn after normal children and receive hit tests first.\n"
            "Hover the button below or the watermark in Overview to see a tooltip.",
            14.0f));
        auto button = std::make_unique<ui::Button>("Hover for tooltip", [] {});
        gallery.tip_button = button.get();
        section->add(std::move(button));
        section->add(std::make_unique<ui::Label>(
            "The debug overlay (F1) is also an overlay: it shows frame time, draw calls, vertices and indices.",
            14.0f));
        section->add(std::make_unique<ui::Button>("Toggle debug overlay", [&gallery]() {
            gallery.debug->toggle();
        }));
        gallery.sections.push_back(section.get());
        root.add(std::move(section));
    }

    // ---------- Layout ----------
    {
        std::unique_ptr<ui::Panel> section{detail::make_section("Layout")};

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
    if (const char* section = std::getenv("COSINE_GALLERY_SECTION")) {
        const int index = std::atoi(section);
        if (index >= 0 && index < static_cast<int>(gallery.sections.size())) {
            gallery.show_section(static_cast<std::size_t>(index));
        }
    }
    return gallery;
}
} // namespace xel::demo
