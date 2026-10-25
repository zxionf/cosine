#include "ui/label.hpp"
#include "ui/menu_bar.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

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

glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.global_bounds();
    return {bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f};
}

void click(xel::ui::UISystem& ui, glm::vec2 pos)
{
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, pos));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, pos));
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1000.0f, 700.0f);

    auto* root_ptr = root.get();
    auto bar = std::make_unique<xel::ui::MenuBar>();
    bar->set_pos(10.0f, 10.0f);

    int new_calls = 0;
    int save_calls = 0;
    int quit_calls = 0;
    int undo_calls = 0;
    int disabled_calls = 0;

    bar->add_menu("File", {
        xel::ui::MenuItem::make("New", [&new_calls] { ++new_calls; }, "Ctrl+N"),
        xel::ui::MenuItem::make("Save", [&save_calls] { ++save_calls; }, "Ctrl+S"),
        xel::ui::MenuItem::separator_item(),
        xel::ui::MenuItem::make("Quit", [&quit_calls] { ++quit_calls; }, "Ctrl+Q"),
    });
    bar->add_menu("Edit", {
        xel::ui::MenuItem::make("Undo", [&undo_calls] { ++undo_calls; }, "Ctrl+Z"),
        xel::ui::MenuItem::make("Disabled", [&disabled_calls] { ++disabled_calls; }, {}, false),
    });
    bar->add_menu("View", {
        xel::ui::MenuItem::make("Zoom in", [] {}, "Ctrl++"),
    });

    assert(bar->menu_count() == 3);
    assert(bar->title(0) == "File");
    assert(bar->items(0).size() == 4);
    assert(bar->items(1)[1].enabled == false);

    auto* bar_ptr = bar.get();
    root->add(std::move(bar));

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(!bar_ptr->is_open());
    const auto bar_bounds = bar_ptr->global_bounds();
    assert(bar_bounds.size.y > 0.0f);

    // 点击 File 打开菜单，overlay 挂载 scrim + popup。
    const float file_x = bar_bounds.pos.x + 6.0f;
    click(ui, {file_x, bar_bounds.pos.y + bar_bounds.size.y * 0.5f});
    ui.tick();
    assert(bar_ptr->is_open());
    assert(bar_ptr->open_menu() == 0);
    assert(root_ptr->overlays().size() == 2);

    // 再次点击同一标题关闭。
    click(ui, {file_x, bar_bounds.pos.y + bar_bounds.size.y * 0.5f});
    ui.tick();
    assert(!bar_ptr->is_open());
    assert(root_ptr->overlays().empty());

    // 打开后点击第一个菜单项触发回调并关闭。
    click(ui, {file_x, bar_bounds.pos.y + bar_bounds.size.y * 0.5f});
    ui.tick();
    assert(bar_ptr->is_open());
    auto* popup = root_ptr->overlays().size() == 2 ? root_ptr->overlays().back().get() : nullptr;
    assert(popup != nullptr);
    const auto popup_bounds = popup->global_bounds();
    // 第一项在 padding 之后一行内。
    click(ui, {popup_bounds.pos.x + popup_bounds.size.x * 0.5f,
               popup_bounds.pos.y + 16.0f});
    ui.tick();
    assert(new_calls == 1);
    assert(!bar_ptr->is_open());
    assert(root_ptr->overlays().empty());

    // 键盘：聚焦菜单栏，Down 打开第一项菜单，Right 切到 Edit，Esc 关闭。
    ui.set_focus(bar_ptr);
    ui.dispatch(key(KeyCode::Down));
    ui.tick();
    assert(bar_ptr->open_menu() == 0);

    ui.dispatch(key(KeyCode::Right));
    ui.tick();
    assert(bar_ptr->open_menu() == 1);

    ui.dispatch(key(KeyCode::Escape));
    ui.tick();
    assert(!bar_ptr->is_open());
    assert(root_ptr->overlays().empty());

    // 重新打开 File：点击标题后 current_ 归位到 0，Down 从 New 移到 Save。
    click(ui, {file_x, bar_bounds.pos.y + bar_bounds.size.y * 0.5f});
    ui.tick();
    assert(bar_ptr->open_menu() == 0);

    ui.dispatch(key(KeyCode::Down));
    ui.dispatch(key(KeyCode::Enter));
    ui.tick();
    assert(save_calls == 1);
    assert(!bar_ptr->is_open());
    // 悬停切换到另一个菜单：模拟点击 Edit 标题打开。
    click(ui, {bar_bounds.pos.x + 90.0f, bar_bounds.pos.y + bar_bounds.size.y * 0.5f});
    ui.tick();
    const int opened = bar_ptr->open_menu();
    assert(opened >= 0);
    bar_ptr->close();
    ui.tick();

    // 禁用项不触发回调。
    bar_ptr->open(1);
    ui.tick();
    auto* edit_popup = root_ptr->overlays().back().get();
    const auto edit_bounds = edit_popup->global_bounds();
    const float row = bar_ptr->ui_theme().control_height;
    click(ui, {edit_bounds.pos.x + edit_bounds.size.x * 0.5f,
               edit_bounds.pos.y + 16.0f + row + row * 0.5f});
    ui.tick();
    assert(disabled_calls == 0);

    // 悬停切换菜单时，popup 内容必须跟着换（不能只改尺寸）。
    bar_ptr->open(0);
    ui.tick();
    const auto bar_rect = bar_ptr->global_bounds();
    const float edit_center_x = bar_rect.pos.x
        + bar_ptr->ui_theme().padding * 2.0f + bar_ptr->ui_theme().text_size * 2.2f + 40.0f;
    xel::event::InputEvent move;
    move.type = InputEvent::Type::MouseMove;
    move.mouse_pos = {edit_center_x, bar_rect.pos.y + bar_rect.size.y * 0.5f};
    ui.dispatch(move);
    ui.tick();
    assert(bar_ptr->open_menu() == 1);

    // 点击第一行：应触发 Edit > Undo，而不是 File > New。
    auto* switched_popup = root_ptr->overlays().back().get();
    const auto switched_bounds = switched_popup->global_bounds();
    const int undo_before = undo_calls;
    const int new_before = new_calls;
    click(ui, {switched_bounds.pos.x + switched_bounds.size.x * 0.5f,
               switched_bounds.pos.y + 16.0f});
    ui.tick();
    assert(undo_calls == undo_before + 1);
    assert(new_calls == new_before);
    assert(!bar_ptr->is_open());

    // popup 上下 padding 对称：高度 = rows + padding*2。
    bar_ptr->open(0);
    ui.tick();
    auto* file_popup = root_ptr->overlays().back().get();
    const float popup_row_h = bar_ptr->ui_theme().control_height;
    const float pad = bar_ptr->ui_theme().padding;
    float expected_h = pad * 2.0f;
    for (const auto& item : bar_ptr->items(0)) expected_h += item.separator ? 9.0f : popup_row_h;
    assert(std::fabs(file_popup->global_bounds().size.y - expected_h) < 0.01f);
    bar_ptr->close();
    ui.tick();

    // 点击空白（scrim）关闭。
    click(ui, {900.0f, 600.0f});
    ui.tick();
    assert(!bar_ptr->is_open());
    assert(root_ptr->overlays().empty());

    std::cout << "menu bar tests passed\n";
    return 0;
}
