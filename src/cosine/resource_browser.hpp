#pragma once

#include "backend/vulkan/renderer.hpp"
#include "texture_cache.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/composites/window.hpp"
#include "ui/core/button.hpp"
#include "ui/core/image.hpp"
#include "ui/core/label.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/text_input.hpp"
#include "ui/root.hpp"
#include "ui/views/file_browser.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace xel::editor
{
// 资源浏览器（CO-5）：独立 Assets 窗口。
// 三栏：分类（固定 110）/ 文件列表（弹性）/ 预览（固定 240），
// 顶部地址栏与搜索，底部选中摘要 + Import。
//
// 只负责浏览、预览与“请求导入”；真正把图片绑定到场景节点由调用方
// 通过 set_on_import 决定。双击文件不会导入。
class ResourceBrowser
{
public:
    ResourceBrowser(ui::UIRoot& root, TextureCache& cache, backend::vulkan::Renderer& renderer);
    ~ResourceBrowser();

    // 打开项目后调用：切换根目录并刷新。
    void set_root_directory(std::string directory);
    const std::string& root_directory() const { return root_directory_; }

    // Import 按钮 / 外部触发；参数是选中文件的绝对路径。
    void set_on_import(std::function<void(const std::string&)> handler);
    // 状态文字回调（可选）：用于编辑器状态栏。未设置时只更新窗口内状态栏。
    void set_on_status(std::function<void(const std::string&)> handler);

    void show();
    void hide();
    bool visible() const;
    void refresh();

private:
    void build();
    void refresh_info();
    void rebuild_address();
    void apply_filter(int index);
    void import_selected();
    void set_status(std::string text);

    ui::Window* window_ = nullptr;
    ui::FileBrowser* list_ = nullptr;
    ui::TextInput* path_input_ = nullptr;
    ui::TextInput* search_input_ = nullptr;
    ui::Image* preview_ = nullptr;
    ui::Label* info_name_ = nullptr;
    ui::Label* info_extra_ = nullptr;
    ui::Label* info_size_ = nullptr;
    ui::Label* info_path_ = nullptr;
    ui::Label* status_line_ = nullptr;
    std::vector<ui::Button*> category_buttons_;

    TextureCache* cache_ = nullptr;
    backend::vulkan::Renderer* renderer_ = nullptr;
    std::string root_directory_{"."};
    int filter_ = 0;
    std::function<void(const std::string&)> on_import_;
    std::function<void(const std::string&)> on_status_;
};
} // namespace xel::editor
