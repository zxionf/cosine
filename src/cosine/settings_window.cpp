#include "settings_window.hpp"

#include "editor.hpp"
#include "editor_settings.hpp"
#include "xel/version.hpp"

#include "ui/core/checkbox.hpp"
#include "ui/core/color_input.hpp"
#include "ui/core/drag_float.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/text_input.hpp"
#include "ui/composites/dropdown.hpp"

#include <format>
#include <utility>

namespace xel::editor
{
SettingsWindow::SettingsWindow(ui::UIRoot& root, Editor& editor) : editor_(&editor)
{
    auto window = std::make_unique<ui::Window>("Settings", glm::vec2{200.0f, 120.0f},
                                               glm::vec2{820.0f, 560.0f});
    window->set_min_size({640.0f, 400.0f});
    window->set_collapsible(false);
    window_ = window.get();
    root.add(std::move(window));
    window_->set_visible(false);
    build();
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::show()
{
    if (window_ != nullptr) window_->set_visible(true);
}

void SettingsWindow::hide()
{
    if (window_ != nullptr) window_->set_visible(false);
}

bool SettingsWindow::visible() const
{
    return window_ != nullptr && window_->visible();
}

void SettingsWindow::select_group(int index)
{
    if (index < 0 || index >= static_cast<int>(groups_.size())) return;
    current_ = index;
    for (std::size_t i = 0; i < group_buttons_.size(); ++i) {
        if (group_buttons_[i] != nullptr) {
            group_buttons_[i]->set_pressed(static_cast<int>(i) == index);
        }
    }
    for (std::size_t i = 0; i < groups_.size(); ++i) {
        if (groups_[i] != nullptr) groups_[i]->set_visible(static_cast<int>(i) == index);
    }
}

void SettingsWindow::build()
{
    ui::Vbox* stage = window_->content();
    stage->set_padding(0.0f);
    stage->set_spacing(0.0f);
    stage->set_cross_align(ui::Align::top_left());

    auto body = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 0.0f);
    body->set_padding(0.0f);
    body->set_spacing(0.0f);
    body->set_size_percent({1.0f, 0.0f});
    body->set_flex_grow(1.0f);
    body->set_cross_align(ui::Align::v_stretch());
    body->set_min_size({0.0f, 0.0f});
    body->set_max_size({10000.0f, 10000.0f});

    auto left = std::make_unique<ui::Vbox>(0.0f, 0.0f, 160.0f, 0.0f);
    left->set_padding(8.0f);
    left->set_spacing(6.0f);
    left->set_cross_align(ui::Align::top_center());
    left->set_min_size({160.0f, 0.0f});
    left->set_max_size({160.0f, 10000.0f});

    constexpr const char* kGroupNames[] = {"General", "Preview", "Export", "About"};
    for (int i = 0; i < 4; ++i) {
        auto button = std::make_unique<ui::Button>(kGroupNames[i], [this, i] {
            select_group(i);
        });
        button->set_min_size({140.0f, 32.0f});
        button->set_max_size({140.0f, 32.0f});
        if (i == 0) button->set_pressed(true);
        group_buttons_.push_back(button.get());
        left->add(std::move(button));
    }
    body->add(std::move(left));

    auto scroll = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    scroll->set_padding(0.0f);
    scroll->set_size_percent({1.0f, 0.0f});
    scroll->set_flex_grow(1.0f);
    scroll->set_min_size({0.0f, 0.0f});
    scroll->set_max_size({10000.0f, 10000.0f});
    scroll_ = scroll.get();
    body->add(std::move(scroll));

    stage->add(std::move(body));

    for (int i = 0; i < 4; ++i) {
        auto group = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
        group->set_padding(12.0f);
        group->set_spacing(10.0f);
        group->set_cross_align(ui::Align::top_left());
        group->set_size_percent({1.0f, 0.0f});
        group->set_min_size({0.0f, 0.0f});
        group->set_max_size({10000.0f, 10000.0f});
        group->set_visible(i == 0);
        ui::Vbox* group_ptr = group.get();
        groups_.push_back(group_ptr);
        scroll_->add(std::move(group));

        switch (i) {
            case 0: build_general(*group_ptr); break;
            case 1: build_preview(*group_ptr); break;
            case 2: build_export(*group_ptr); break;
            default: build_about(*group_ptr); break;
        }
    }
}

void SettingsWindow::build_general(ui::Vbox& group)
{
    auto& settings = editor_->settings();
    group.add(std::make_unique<ui::Label>("Appearance", 20.0f));

    group.add(std::make_unique<ui::Checkbox>(
        "Rounded style", settings.rounded_style, [this](bool checked) {
            editor_->settings().rounded_style = checked;
            editor_->apply_settings();
        }));
    group.add(std::make_unique<ui::Checkbox>(
        "Light theme", settings.light_theme, [this](bool checked) {
            editor_->settings().light_theme = checked;
            editor_->apply_settings();
        }));

    auto row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    row->set_padding(0.0f);
    row->set_spacing(8.0f);
    row->set_cross_align(ui::Align::center_left());
    row->add(std::make_unique<ui::Label>("UI text size", 16.0f));
    auto drag = std::make_unique<ui::DragFloat>("", 120.0f);
    drag->set_range(14.0f, 36.0f);
    drag->set_value(settings.ui_text_size);
    drag->set_on_change([this](float value) {
        editor_->settings().ui_text_size = value;
        editor_->apply_settings();
    });
    row->add(std::move(drag));
    group.add(std::move(row));
}

void SettingsWindow::build_preview(ui::Vbox& group)
{
    auto& settings = editor_->settings();
    group.add(std::make_unique<ui::Label>("Preview", 20.0f));

    auto size_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    size_row->set_padding(0.0f);
    size_row->set_spacing(8.0f);
    size_row->set_cross_align(ui::Align::center_left());
    size_row->add(std::make_unique<ui::Label>("Canvas", 16.0f));
    auto width = std::make_unique<ui::DragFloat>("", 100.0f);
    width->set_range(16.0f, 8192.0f);
    width->set_value(settings.preview_canvas.x);
    width->set_on_change([this](float value) {
        editor_->settings().preview_canvas.x = value;
    });
    size_row->add(std::move(width));
    auto height = std::make_unique<ui::DragFloat>("", 100.0f);
    height->set_range(16.0f, 8192.0f);
    height->set_value(settings.preview_canvas.y);
    height->set_on_change([this](float value) {
        editor_->settings().preview_canvas.y = value;
    });
    size_row->add(std::move(height));
    group.add(std::move(size_row));

    auto fit = std::make_unique<ui::Dropdown>(
        std::vector<std::string>{"Contain", "Cover", "Fill"}, settings.preview_fit);
    fit->set_on_change([this](int index) { editor_->settings().preview_fit = index; });
    group.add(std::move(fit));

    auto background = std::make_unique<ui::ColorInput>("Background", 250.0f);
    background->set_value(settings.preview_background);
    background->set_on_change([this](glm::vec4 color) {
        editor_->settings().preview_background = color;
        editor_->apply_settings();
    });
    group.add(std::move(background));

    group.add(std::make_unique<ui::Checkbox>(
        "Show ruler", settings.show_ruler, [this](bool checked) {
            editor_->settings().show_ruler = checked;
            editor_->apply_settings();
        }));
}

void SettingsWindow::build_export(ui::Vbox& group)
{
    auto& settings = editor_->settings();
    group.add(std::make_unique<ui::Label>("Export", 20.0f));

    auto size_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    size_row->set_padding(0.0f);
    size_row->set_spacing(8.0f);
    size_row->set_cross_align(ui::Align::center_left());
    size_row->add(std::make_unique<ui::Label>("Size", 16.0f));
    auto width = std::make_unique<ui::DragFloat>("", 100.0f);
    width->set_range(16.0f, 8192.0f);
    width->set_value(settings.export_size.x);
    width->set_on_change([this](float value) {
        editor_->settings().export_size.x = value;
    });
    size_row->add(std::move(width));
    auto height = std::make_unique<ui::DragFloat>("", 100.0f);
    height->set_range(16.0f, 8192.0f);
    height->set_value(settings.export_size.y);
    height->set_on_change([this](float value) {
        editor_->settings().export_size.y = value;
    });
    size_row->add(std::move(height));
    group.add(std::move(size_row));

    auto fps_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    fps_row->set_padding(0.0f);
    fps_row->set_spacing(8.0f);
    fps_row->set_cross_align(ui::Align::center_left());
    fps_row->add(std::make_unique<ui::Label>("FPS", 16.0f));
    auto fps = std::make_unique<ui::DragFloat>("", 100.0f);
    fps->set_range(1.0f, 120.0f);
    fps->set_value(static_cast<float>(settings.export_fps));
    fps->set_on_change([this](float value) {
        editor_->settings().export_fps = static_cast<int>(value + 0.5f);
    });
    fps_row->add(std::move(fps));
    group.add(std::move(fps_row));

    auto background = std::make_unique<ui::ColorInput>("Background", 250.0f);
    background->set_value(settings.export_background);
    background->set_on_change([this](glm::vec4 color) {
        editor_->settings().export_background = color;
    });
    group.add(std::move(background));

    auto dir_row = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    dir_row->set_padding(0.0f);
    dir_row->set_spacing(8.0f);
    dir_row->set_cross_align(ui::Align::center_left());
    dir_row->add(std::make_unique<ui::Label>("Output", 16.0f));
    auto directory = std::make_unique<ui::TextInput>(settings.export_directory, 360.0f);
    directory->set_on_change([this](const std::string& text) {
        editor_->settings().export_directory = text;
    });
    dir_row->add(std::move(directory));
    group.add(std::move(dir_row));
}

void SettingsWindow::build_about(ui::Vbox& group)
{
    group.add(std::make_unique<ui::Label>("About", 20.0f));
    group.add(std::make_unique<ui::Label>(
        std::format("cosine {}", xel::version_string()), 16.0f));

#if defined(_WIN32)
    constexpr const char* kPlatform = "Windows";
#elif defined(__APPLE__)
    constexpr const char* kPlatform = "macOS";
#elif defined(__linux__)
    constexpr const char* kPlatform = "Linux";
#else
    constexpr const char* kPlatform = "Unknown";
#endif

#if defined(__clang__)
    const std::string compiler =
        std::format("Clang {}.{}", __clang_major__, __clang_minor__);
#elif defined(__GNUC__)
    const std::string compiler = std::format("GCC {}.{}", __GNUC__, __GNUC_MINOR__);
#elif defined(_MSC_VER)
    const std::string compiler = std::format("MSVC {}", _MSC_VER);
#else
    const std::string compiler = "Unknown";
#endif

#ifdef NDEBUG
    constexpr const char* kBuild = "Release";
#else
    constexpr const char* kBuild = "Debug";
#endif

    group.add(std::make_unique<ui::Label>(std::format("Platform: {}", kPlatform), 14.0f));
    group.add(std::make_unique<ui::Label>("Compiler: " + compiler, 14.0f));
    group.add(std::make_unique<ui::Label>(std::format("Build: {}", kBuild), 14.0f));
    const std::string& path = editor_->project_path();
    group.add(std::make_unique<ui::Label>(
        "Project: " + (path.empty() ? std::string{"(unsaved)"} : path), 14.0f));
}
} // namespace xel::editor
