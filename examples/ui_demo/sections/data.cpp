// 数据视图：Table 与 TreeView。
#include "sections/sections.hpp"

#include "ui/core/label.hpp"
#include "ui/core/typography.hpp"
#include "ui/views/file_browser.hpp"
#include "ui/views/table.hpp"
#include "ui/views/tree_view.hpp"

#include <memory>
#include <string>
#include <vector>

namespace demo::sections
{
namespace
{
std::vector<std::vector<std::string>> make_rows(int count)
{
    std::vector<std::vector<std::string>> rows;
    rows.reserve(static_cast<std::size_t>(count));
    const char* kinds[] = {"image", "audio", "mesh", "shader"};
    for (int i = 0; i < count; ++i) {
        rows.push_back({
            "asset_" + std::to_string(i),
            kinds[i % 4],
            std::to_string(8 + i * 3) + " KB",
            (i % 5 == 0) ? "dirty" : "ok",
        });
    }
    return rows;
}
} // namespace

void build_data(Demo& demo, ui::Vbox& column)
{
    auto& table = group(column, "Table：固定表头 + 滚动行");
    hint(table,
         "列宽可弹性（<=0）或固定；斑马纹帮助横向追踪；点击行选中，键盘上下也能移动。");
    auto view = std::make_unique<ui::Table>(480.0f, 220.0f);
    // 宽度交给父级（fill_x），高度固定：Table 是 fixed_size 控件，靠 min/max 决定尺寸。
    view->set_min_size({0.0f, 220.0f});
    view->set_max_size({10000.0f, 220.0f});
    view->set_columns({
        {"名称", 0.0f, ui::TableColumn::Align::Left},
        {"类型", 110.0f, ui::TableColumn::Align::Left},
        {"大小", 100.0f, ui::TableColumn::Align::Right},
        {"状态", 90.0f, ui::TableColumn::Align::Center},
    });
    view->set_rows(make_rows(40));
    view->set_striped(true);
    view->set_selected(0);
    view->set_fill(true, false);
    demo.note("数据分区：滚轮滚动表格，点击行切换选中");
    table.add(std::move(view));

    auto& tree = group(column, "TreeView：展开 / 选中 / 键盘");
    hint(tree,
         "树节点按路径标识；左/右方向键折叠展开，回车选中。展开状态与选中项在切走时保留。");
    auto tree_view = std::make_unique<ui::TreeView>(320.0f, 200.0f);
    tree_view->set_min_size({0.0f, 200.0f});
    tree_view->set_max_size({10000.0f, 200.0f});
    ui::TreeNode root;
    root.title = "Project";
    root.expanded = true;
    for (const char* child_name : {"assets", "scenes", "scripts"}) {
        ui::TreeNode child;
        child.title = child_name;
        child.expanded = true;
        for (const char* leaf : {"a", "b"}) {
            ui::TreeNode item;
            item.title = std::string{child_name} + "/" + leaf;
            child.children.push_back(std::move(item));
        }
        root.children.push_back(std::move(child));
    }
    tree_view->set_roots({std::move(root)});
    tree_view->set_on_select([&demo](const std::vector<int>& path) {
        std::string text = "选中路径:";
        for (int index : path) text += " " + std::to_string(index);
        demo.note(text);
    });
    tree_view->set_fill(true, false);
    tree.add(std::move(tree_view));

    auto& browser = group(column, "FileBrowser：目录 + 扩展名过滤");
    hint(browser, "对文件系统只读；演示 Table 复用与目录导航的上层封装。");
    auto file_browser = std::make_unique<ui::FileBrowser>(glm::vec2{0.0f, 210.0f});
    file_browser->set_extension_filter(".cpp");
    browser.add(std::move(file_browser));
    browser.add(std::make_unique<ui::Paragraph>(
        "上面是 FileBrowser（默认读当前工作目录，扩展名过滤 .cpp）。"
        "它把目录读取、排序、双击进入等逻辑收在控件内部，外壳只负责摆放。"));
}
} // namespace demo::sections
