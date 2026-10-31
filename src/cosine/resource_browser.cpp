#include "resource_browser.hpp"

#include "ui/views/file_browser.hpp"

#include <stb_image.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <utility>

namespace xel::editor
{
namespace
{
constexpr const char* kFilters[] = {"All", "Images", "Fonts", "Projects"};
constexpr const char* kExtensions[] = {"", ".png", ".ttf", ".cos"};

constexpr float kTopHeight = 40.0f;
constexpr float kBottomHeight = 40.0f;
constexpr float kLeftWidth = 110.0f;
constexpr float kRightWidth = 240.0f;
} // namespace

ResourceBrowser::ResourceBrowser(ui::UIRoot& root, TextureCache& cache,
                                 backend::vulkan::Renderer& renderer)
    : cache_{&cache}, renderer_{&renderer}
{
    std::error_code dir_ec;
    const std::filesystem::path cwd = std::filesystem::current_path(dir_ec);
    root_directory_ = dir_ec ? std::string{"."} : cwd.string();

    auto window = std::make_unique<ui::Window>("Assets", glm::vec2{300.0f, 24.0f},
                                               glm::vec2{880.0f, 520.0f});
    window->set_min_size({800.0f, 360.0f});
    window->set_collapsible(false);
    window_ = window.get();
    root.add(std::move(window));
    window_->set_visible(false);

    build();
}

ResourceBrowser::~ResourceBrowser() = default;

void ResourceBrowser::set_root_directory(std::string directory)
{
    root_directory_ = std::move(directory);
    if (root_directory_.empty()) root_directory_ = ".";
    if (list_ != nullptr) {
        list_->set_directory(root_directory_);
        if (search_input_ != nullptr) list_->set_name_filter(search_input_->text());
        if (path_input_ != nullptr) path_input_->set_text(list_->directory());
        if (list_->entry_count() > 1) list_->select_index(1);
        refresh_info();
    }
}

void ResourceBrowser::set_on_import(std::function<void(const std::string&)> handler)
{
    on_import_ = std::move(handler);
}

void ResourceBrowser::set_on_status(std::function<void(const std::string&)> handler)
{
    on_status_ = std::move(handler);
}

void ResourceBrowser::show()
{
    if (window_ != nullptr) window_->set_visible(true);
}

void ResourceBrowser::hide()
{
    if (window_ != nullptr) window_->set_visible(false);
}

bool ResourceBrowser::visible() const
{
    return window_ != nullptr && window_->visible();
}

void ResourceBrowser::refresh()
{
    if (list_ == nullptr) return;
    list_->set_directory(list_->directory());
    if (path_input_ != nullptr) path_input_->set_text(list_->directory());
    if (list_->entry_count() > 1) list_->select_index(1);
    refresh_info();
}

void ResourceBrowser::set_status(std::string text)
{
    if (status_line_ != nullptr) status_line_->set_text(text);
    if (on_status_) on_status_(std::move(text));
}

void ResourceBrowser::build()
{
    ui::Vbox* stage = window_->content();
    stage->set_padding(0.0f);
    stage->set_spacing(0.0f);
    stage->set_cross_align(ui::Align::top_left());

    // 顶部：Up / 目录 / 搜索 / Go。
    auto top = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, kTopHeight);
    top->set_padding(ui::Edges{8.0f, 0.0f, 8.0f, 0.0f});
    top->set_spacing(4.0f);
    top->set_size_percent({1.0f, 0.0f});
    top->set_min_size({0.0f, kTopHeight});
    top->set_max_size({10000.0f, kTopHeight});
    top->set_cross_align(ui::Align::center_left());

    auto up = std::make_unique<ui::Button>("Up", [this] {
        if (list_ == nullptr) return;
        const std::filesystem::path current(list_->directory());
        if (!current.has_parent_path() || current.parent_path() == current) return;
        list_->set_directory(current.parent_path().string());
        rebuild_address();
        refresh_info();
    });
    up->set_min_size({48.0f, 28.0f});
    up->set_max_size({48.0f, 28.0f});
    top->add(std::move(up));

    auto path = std::make_unique<ui::TextInput>("", 460.0f);
    path->set_min_size({0.0f, 28.0f});
    path->set_max_size({10000.0f, 28.0f});
    path->set_flex_grow(1.0f);
    path_input_ = path.get();
    top->add(std::move(path));

    auto search = std::make_unique<ui::TextInput>("", 160.0f);
    search->set_min_size({0.0f, 28.0f});
    search->set_max_size({10000.0f, 28.0f});
    search->set_on_change([this](const std::string& text) {
        if (list_ != nullptr) list_->set_name_filter(text);
        refresh_info();
    });
    search_input_ = search.get();
    top->add(std::move(search));

    auto go = std::make_unique<ui::Button>("Go", [this] {
        if (path_input_ == nullptr || list_ == nullptr) return;
        std::error_code path_ec;
        const std::filesystem::path target(path_input_->text());
        if (!std::filesystem::is_directory(target, path_ec)) {
            set_status("not a directory: " + target.string());
            return;
        }
        list_->set_directory(target.string());
        rebuild_address();
        refresh_info();
    });
    go->set_min_size({40.0f, 28.0f});
    go->set_max_size({40.0f, 28.0f});
    top->add(std::move(go));

    stage->add(std::move(top));

    // 中间：左分类 / 中列表 / 右预览。
    auto middle = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, 0.0f);
    middle->set_padding(0.0f);
    middle->set_spacing(12.0f);
    middle->set_size_percent({1.0f, 0.0f});
    middle->set_min_size({0.0f, 0.0f});
    middle->set_max_size({10000.0f, 10000.0f});
    middle->set_flex_grow(1.0f);
    middle->set_cross_align(ui::Align::v_stretch());

    auto left = std::make_unique<ui::Vbox>(0.0f, 0.0f, kLeftWidth, 0.0f);
    left->set_padding(0.0f);
    left->set_spacing(4.0f);
    left->set_cross_align(ui::Align::top_center());
    left->set_min_size({kLeftWidth, 0.0f});
    left->set_max_size({kLeftWidth, 10000.0f});
    left->add(std::make_unique<ui::Label>("Library", 16.0f));
    for (int i = 0; i < 4; ++i) {
        auto button = std::make_unique<ui::Button>(kFilters[i], [this, i] {
            apply_filter(i);
            refresh_info();
        });
        button->set_min_size({100.0f, 28.0f});
        button->set_max_size({100.0f, 28.0f});
        if (i == 0) button->set_pressed(true);
        category_buttons_.push_back(button.get());
        left->add(std::move(button));
    }
    middle->add(std::move(left));

    auto center = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    center->set_padding(0.0f);
    center->set_spacing(0.0f);
    center->set_cross_align(ui::Align::h_stretch());
    center->set_min_size({0.0f, 0.0f});
    center->set_max_size({10000.0f, 10000.0f});
    center->set_flex_grow(1.0f);

    auto list = std::make_unique<ui::FileBrowser>(glm::vec2{430.0f, 300.0f});
    list_ = list.get();
    list->set_header_height(28.0f);
    list->set_auto_width(true);
    list->set_flex_grow(1.0f);
    list->set_min_size({240.0f, 0.0f});
    list->set_max_size({10000.0f, 10000.0f});
    list->set_directory(root_directory_);
    list->set_extension_filter(kExtensions[filter_]);
    list->set_on_select([this] { refresh_info(); });
    // 双击文件不导入；目录双击由 FileBrowser 内部进入并触发 directory_changed。
    list->set_on_activate([this] { refresh_info(); });
    list->set_on_directory_changed([this] {
        rebuild_address();
        refresh_info();
    });
    center->add(std::move(list));
    middle->add(std::move(center));

    auto right = std::make_unique<ui::Vbox>(0.0f, 0.0f, kRightWidth, 0.0f);
    right->set_padding(0.0f);
    right->set_spacing(6.0f);
    right->set_cross_align(ui::Align::top_left());
    right->set_min_size({kRightWidth, 0.0f});
    right->set_max_size({kRightWidth, 10000.0f});
    auto preview = std::make_unique<ui::Image>(200.0f, 150.0f);
    preview_ = preview.get();
    right->add(std::move(preview));
    auto name = std::make_unique<ui::Label>("-", 16.0f);
    info_name_ = name.get();
    right->add(std::move(name));
    auto extra = std::make_unique<ui::Label>("", 16.0f);
    info_extra_ = extra.get();
    right->add(std::move(extra));
    auto size = std::make_unique<ui::Label>("", 16.0f);
    info_size_ = size.get();
    right->add(std::move(size));
    auto path_label = std::make_unique<ui::Label>("", 16.0f);
    info_path_ = path_label.get();
    right->add(std::move(path_label));
    middle->add(std::move(right));

    stage->add(std::move(middle));

    // 底部：选中摘要 + Import。
    auto bottom = std::make_unique<ui::Hbox>(0.0f, 0.0f, 0.0f, kBottomHeight);
    bottom->set_padding(ui::Edges{8.0f, 0.0f, 8.0f, 0.0f});
    bottom->set_spacing(8.0f);
    bottom->set_min_size({0.0f, kBottomHeight});
    bottom->set_max_size({10000.0f, kBottomHeight});
    bottom->set_cross_align(ui::Align::center_left());

    auto status = std::make_unique<ui::Label>("no selection", 16.0f);
    status_line_ = status.get();
    bottom->add(std::move(status));

    auto spacer = std::make_unique<ui::Spacer>(1.0f, 1.0f);
    spacer->set_flex_grow(1.0f);
    bottom->add(std::move(spacer));

    auto import = std::make_unique<ui::Button>("Import", [this] { import_selected(); });
    import->set_min_size({0.0f, 28.0f});
    import->set_max_size({1000.0f, 28.0f});
    bottom->add(std::move(import));

    window_->set_footer(std::move(bottom));

    rebuild_address();
    if (list_->entry_count() > 1) list_->select_index(1);
    refresh_info();
}

void ResourceBrowser::apply_filter(int index)
{
    filter_ = index;
    for (std::size_t i = 0; i < category_buttons_.size(); ++i) {
        if (category_buttons_[i] != nullptr) {
            category_buttons_[i]->set_pressed(static_cast<int>(i) == index);
        }
    }
    if (list_ != nullptr) list_->set_extension_filter(kExtensions[index]);
}

void ResourceBrowser::rebuild_address()
{
    if (path_input_ != nullptr && list_ != nullptr) {
        path_input_->set_text(list_->directory());
    }
}

void ResourceBrowser::import_selected()
{
    if (list_ == nullptr) return;
    if (list_->selected_name().empty() || list_->selected_is_directory()) {
        set_status("select a file to import");
        return;
    }
    const std::string path = list_->selected_path();
    if (path.empty()) return;
    if (on_import_) on_import_(path);
}

void ResourceBrowser::refresh_info()
{
    if (list_ == nullptr) return;
    const std::string& name = list_->selected_name();
    if (name.empty()) {
        if (info_name_ != nullptr) info_name_->set_text("-");
        if (info_size_ != nullptr) info_size_->set_text("");
        if (info_extra_ != nullptr) info_extra_->set_text("");
        if (info_path_ != nullptr) info_path_->set_text("");
        if (status_line_ != nullptr) status_line_->set_text("no selection");
        if (preview_ != nullptr) {
            preview_->set_texture_id(0);
            preview_->set_source("");
            preview_->set_placeholder_text("image");
        }
        return;
    }

    const bool is_dir = list_->selected_is_directory();
    const std::string path = list_->selected_path();
    const std::string size_text =
        is_dir ? std::string{"directory"}
               : ui::format_bytes(list_->entry_size(
                     static_cast<std::size_t>(list_->selected_index())));

    std::string extra;
    bool has_texture = false;
    if (!is_dir && path.size() > 4 && path.compare(path.size() - 4, 4, ".png") == 0) {
        int w = 0;
        int h = 0;
        int channels = 0;
        if (stbi_info(path.c_str(), &w, &h, &channels) != 0) {
            extra = std::format("{} × {} px", w, h);
            if (preview_ != nullptr && cache_ != nullptr && renderer_ != nullptr) {
                const std::uint32_t id = cache_->load(path, *renderer_);
                if (id != 0) {
                    preview_->set_texture_id(id);
                    preview_->set_texture_size(
                        {static_cast<float>(w), static_cast<float>(h)});
                    preview_->set_fit(ui::Image::Fit::Contain);
                    has_texture = true;
                }
            }
        }
    }
    if (preview_ != nullptr && !has_texture) {
        preview_->set_texture_id(0);
    }

    std::string name_text = name + (is_dir ? "/" : "");
    if (name_text.size() > 22) name_text = name_text.substr(0, 20) + "...";
    if (info_name_ != nullptr) info_name_->set_text(name_text);
    if (info_extra_ != nullptr) info_extra_->set_text(extra);
    if (info_size_ != nullptr) info_size_->set_text(size_text);
    if (info_path_ != nullptr) {
        std::string path_text = path;
        if (path_text.size() > 24) path_text = "..." + path_text.substr(path_text.size() - 21);
        info_path_->set_text(path_text);
    }
    if (preview_ != nullptr) {
        preview_->set_source(path);
        preview_->set_placeholder_text(name + (is_dir ? "/" : ""));
    }
    if (status_line_ != nullptr) {
        std::string summary = name + (is_dir ? "/" : "");
        if (!extra.empty()) summary += "  |  " + extra;
        if (!size_text.empty()) summary += "  |  " + size_text;
        status_line_->set_text(summary);
    }
}
} // namespace xel::editor
