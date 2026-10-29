#include "ui/views/tree_view.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
using xel::event::InputEvent;
using xel::event::KeyCode;

InputEvent mouse(InputEvent::Type type, glm::vec2 pos)
{
    InputEvent event;
    event.type = type;
    event.mouse_pos = pos;
    return event;
}

InputEvent key(KeyCode code)
{
    InputEvent event;
    event.type = InputEvent::Type::KeyDown;
    event.key = code;
    return event;
}

void click(xel::ui::UISystem& ui, glm::vec2 pos)
{
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, pos));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, pos));
}

xel::ui::TreeNode node(std::string title)
{
    xel::ui::TreeNode value;
    value.title = std::move(title);
    return value;
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 900.0f, 600.0f);

    auto tree = std::make_unique<xel::ui::TreeView>(320.0f, 200.0f);
    tree->set_pos(20.0f, 20.0f);

    // src/{main.cpp, ui/{widget.hpp, tree_view.hpp}}, docs/{spec.md}, README.md
    xel::ui::TreeNode src = node("src");
    xel::ui::TreeNode ui_dir = node("ui");
    ui_dir.children.push_back(node("widget.hpp"));
    ui_dir.children.push_back(node("tree_view.hpp"));
    src.children.push_back(node("main.cpp"));
    src.children.push_back(std::move(ui_dir));

    xel::ui::TreeNode docs = node("docs");
    docs.children.push_back(node("spec.md"));

    std::vector<xel::ui::TreeNode> roots;
    roots.push_back(std::move(src));
    roots.push_back(std::move(docs));
    roots.push_back(node("README.md"));
    tree->set_roots(std::move(roots));

    int select_calls = 0;
    std::vector<int> last_path;
    tree->set_on_select([&](const std::vector<int>& path) {
        ++select_calls;
        last_path = path;
    });

    auto* tree_ptr = tree.get();
    root->add(std::move(tree));

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    const auto bounds = tree_ptr->global_bounds();
    const float row_h = tree_ptr->row_height();
    const float pad = xel::ui::theme().padding;
    const float indent = tree_ptr->indent();

    assert(tree_ptr->root_count() == 3);
    assert(tree_ptr->visible_count() == 3);
    assert(tree_ptr->selected_path().empty());

    // 点击行选中（第三行 README.md）。
    click(ui, {bounds.pos.x + 100.0f, bounds.pos.y + row_h * 2.5f});
    assert(tree_ptr->selected_path() == std::vector<int>{2});
    assert(select_calls == 1);
    assert(last_path == std::vector<int>{2});

    // 点击 src 的箭头展开：不改选中，只改可见行。
    const float root_arrow_x = bounds.pos.x + pad + 7.0f;
    click(ui, {root_arrow_x, bounds.pos.y + row_h * 0.5f});
    assert(tree_ptr->is_expanded({0}));
    assert(tree_ptr->visible_count() == 5);
    assert(tree_ptr->selected_path() == std::vector<int>{2});

    // 展开 ui（第 3 行，depth 1）：多出两个子节点。
    const float child_arrow_x = bounds.pos.x + pad + indent + 7.0f;
    click(ui, {child_arrow_x, bounds.pos.y + row_h * 2.5f});
    assert(tree_ptr->is_expanded({0, 1}));
    assert(tree_ptr->visible_count() == 7);

    // 点击折叠箭头。
    click(ui, {child_arrow_x, bounds.pos.y + row_h * 2.5f});
    assert(!tree_ptr->is_expanded({0, 1}));
    assert(tree_ptr->visible_count() == 5);
    tree_ptr->set_expanded({0, 1}, true);

    // 键盘导航。
    ui.set_focus(tree_ptr);
    ui.dispatch(key(KeyCode::Home));
    assert(tree_ptr->selected_path() == std::vector<int>{0});
    ui.dispatch(key(KeyCode::Down));
    assert(tree_ptr->selected_path() == std::vector<int>{0, 0});
    ui.dispatch(key(KeyCode::End));
    assert(tree_ptr->selected_path() == std::vector<int>{2});
    ui.dispatch(key(KeyCode::Up));
    assert(tree_ptr->selected_path() == std::vector<int>{1});
    ui.dispatch(key(KeyCode::Home));

    // Right：已展开则进入第一个子节点。
    ui.dispatch(key(KeyCode::Right));
    assert(tree_ptr->selected_path() == std::vector<int>{0, 0});
    // Left：折叠状态下跳到父节点。
    ui.dispatch(key(KeyCode::Left));
    assert(tree_ptr->selected_path() == std::vector<int>{0});
    // Left：已展开则折叠当前节点。
    ui.dispatch(key(KeyCode::Left));
    assert(!tree_ptr->is_expanded({0}));
    assert(tree_ptr->visible_count() == 3);
    // Right：折叠状态下只展开，不移动选中。
    ui.dispatch(key(KeyCode::Right));
    assert(tree_ptr->is_expanded({0}));
    assert(tree_ptr->selected_path() == std::vector<int>{0});

    // Space / Enter 切换当前节点。
    ui.dispatch(key(KeyCode::Space));
    assert(!tree_ptr->is_expanded({0}));
    ui.dispatch(key(KeyCode::Enter));
    assert(tree_ptr->is_expanded({0}));

    // 折叠父级时，子树内的选中上移到被折叠节点。
    tree_ptr->set_expanded({0, 1}, true);
    tree_ptr->set_selected_path({0, 1, 0});
    assert(tree_ptr->selected_node() && tree_ptr->selected_node()->title == "widget.hpp");
    tree_ptr->set_expanded({0}, false);
    assert(tree_ptr->selected_path() == std::vector<int>{0});

    // 无子节点的行按 Enter 不产生变化。
    tree_ptr->set_selected_path({2});
    ui.dispatch(key(KeyCode::Enter));
    assert(tree_ptr->selected_path() == std::vector<int>{2});

    // 自动保持选中可见：追加大量根节点后 End 选中最后一行，应滚动。
    for (int i = 0; i < 30; ++i) {
        tree_ptr->add_root(node("item_" + std::to_string(i)));
    }
    assert(tree_ptr->visible_count() == 33);
    assert(tree_ptr->scroll_offset() == 0.0f);
    ui.dispatch(key(KeyCode::End));
    assert(tree_ptr->scroll_offset() > 0.0f);

    // 清空后键盘事件不崩，选中归空。
    tree_ptr->clear_roots();
    assert(tree_ptr->visible_count() == 0);
    assert(tree_ptr->selected_path().empty());
    ui.dispatch(key(KeyCode::Down));
    ui.dispatch(key(KeyCode::Right));
    assert(tree_ptr->visible_count() == 0);

    std::cout << "tree view tests passed\n";
    return 0;
}
