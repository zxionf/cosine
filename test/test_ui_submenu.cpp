#include "events/key_code.hpp"
#include "ui/composites/menu_bar.hpp"
#include "ui/root.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
using namespace xel;

struct Fixture
{
    ui::MenuBar* bar = nullptr;
    std::unique_ptr<ui::UISystem> ui;
    int new_count = 0;
    int png_count = 0;
    int mp4_count = 0;

    Fixture()
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
        auto bar_owner = std::make_unique<ui::MenuBar>();
        bar = bar_owner.get();

        bar->add_menu("File", {
            ui::MenuItem::make("New", [this]() { ++new_count; }),
            ui::MenuItem::submenu("Export", {
                ui::MenuItem::make("PNG", [this]() { ++png_count; }),
                ui::MenuItem::submenu("Video", {
                    ui::MenuItem::make("MP4", [this]() { ++mp4_count; }),
                }),
            }),
        });
        bar->add_menu("Edit", {
            ui::MenuItem::make("Undo", [this]() { ++new_count; }),
        });

        root->add(std::move(bar_owner));
        ui = std::make_unique<ui::UISystem>(std::move(root));
        ui->tick();
    }

    ui::detail::MenuPopup* popup_at(std::size_t index) const
    {
        const auto& overlays = ui->root()->overlays();
        if (index >= overlays.size()) return nullptr;
        return dynamic_cast<ui::detail::MenuPopup*>(overlays[index].get());
    }

    std::size_t overlay_count() const { return ui->root()->overlays().size(); }
};

void test_children_structure()
{
    Fixture f;
    const auto& file = f.bar->items(0);
    assert(file.size() == 2);
    assert(!file[0].has_submenu());
    assert(file[1].has_submenu());
    assert(file[1].children.size() == 2);
    assert(file[1].children[1].has_submenu());
    assert(file[1].children[1].children.size() == 1);
    assert(file[1].children[1].children[0].label == "MP4");
}

void test_hover_opens_submenus_level_by_level()
{
    Fixture f;
    f.bar->open(0);
    f.ui->tick();
    assert(f.overlay_count() == 2); // scrim + top popup
    assert(f.bar->submenu_depth() == 0);

    auto* top = f.popup_at(1);
    assert(top != nullptr);
    const auto top_bounds = top->global_bounds();
    top->on_mouse_move({top_bounds.pos.x + 10.0f, top->item_top(1) + 5.0f});
    assert(f.overlay_count() == 3); // + Export
    assert(f.bar->submenu_depth() == 1);

    auto* export_popup = f.popup_at(2);
    assert(export_popup != nullptr);
    const auto export_bounds = export_popup->global_bounds();
    // 子菜单应贴在父菜单右侧。
    assert(export_bounds.pos.x >= top_bounds.pos.x + top_bounds.size.x - 8.0f);
    export_popup->on_mouse_move({export_bounds.pos.x + 10.0f, export_popup->item_top(1) + 5.0f});
    assert(f.overlay_count() == 4); // + Video
    assert(f.bar->submenu_depth() == 2);

    // 悬停回无子菜单的项：收起更深层。
    top->on_mouse_move({top_bounds.pos.x + 10.0f, top->item_top(0) + 5.0f});
    assert(f.overlay_count() == 2);
    assert(f.bar->submenu_depth() == 0);
}

void test_click_leaf_runs_action_and_closes_all()
{
    Fixture f;
    f.bar->open(0);
    f.ui->tick();

    auto* top = f.popup_at(1);
    const auto top_bounds = top->global_bounds();
    top->on_mouse_move({top_bounds.pos.x + 10.0f, top->item_top(1) + 5.0f});

    auto* export_popup = f.popup_at(2);
    const auto export_bounds = export_popup->global_bounds();
    export_popup->on_mouse_move({export_bounds.pos.x + 10.0f, export_popup->item_top(1) + 5.0f});

    auto* video_popup = f.popup_at(3);
    const auto video_bounds = video_popup->global_bounds();
    assert(video_popup->on_click({video_bounds.pos.x + 10.0f, video_popup->item_top(0) + 5.0f}, 0));

    assert(f.mp4_count == 1);
    assert(f.png_count == 0);
    assert(f.overlay_count() == 0);
    assert(!f.bar->is_open());
}

void test_keyboard_enter_and_leave_submenus()
{
    Fixture f;
    f.bar->open(0);
    f.ui->tick();

    auto* top = f.popup_at(1);
    top->set_keyboard_highlight(1); // Export
    assert(f.bar->on_key_down(event::KeyCode::Right, event::Modifier::None));
    assert(f.bar->submenu_depth() == 1);
    assert(f.overlay_count() == 3);

    // 新层的高亮落在第一个可执行项。
    auto* export_popup = f.popup_at(2);
    assert(export_popup->keyboard_highlight() == 0);

    // Right 进入 Video，Left 只退一层。
    export_popup->set_keyboard_highlight(1);
    assert(f.bar->on_key_down(event::KeyCode::Right, event::Modifier::None));
    assert(f.bar->submenu_depth() == 2);
    assert(f.overlay_count() == 4);

    assert(f.bar->on_key_down(event::KeyCode::Left, event::Modifier::None));
    assert(f.bar->submenu_depth() == 1);
    assert(f.overlay_count() == 3);

    assert(f.bar->on_key_down(event::KeyCode::Left, event::Modifier::None));
    assert(f.bar->submenu_depth() == 0);
    assert(f.overlay_count() == 2);

    // Esc 关闭整条链。
    assert(f.bar->on_key_down(event::KeyCode::Escape, event::Modifier::None));
    assert(f.overlay_count() == 0);
    assert(!f.bar->is_open());
}

void test_enter_on_leaf_closes()
{
    Fixture f;
    f.bar->open(0);
    f.ui->tick();

    auto* top = f.popup_at(1);
    top->set_keyboard_highlight(0); // New
    assert(f.bar->on_key_down(event::KeyCode::Enter, event::Modifier::None));
    assert(f.new_count == 1);
    assert(f.overlay_count() == 0);
}

void test_focus_loss_closes_menu()
{
    Fixture f;
    f.ui->set_focus(f.bar);
    f.bar->open(0);
    f.ui->tick();
    assert(f.bar->is_open());

    // 焦点转到别处（如 Tab 离开菜单栏）时菜单必须收起。
    f.ui->set_focus(nullptr);
    assert(!f.bar->is_open());
    assert(f.overlay_count() == 0);
}

} // namespace

int main()
{
    test_children_structure();
    test_hover_opens_submenus_level_by_level();
    test_click_leaf_runs_action_and_closes_all();
    test_keyboard_enter_and_leave_submenus();
    test_enter_on_leaf_closes();
    test_focus_loss_closes_menu();
    std::cout << "ui submenu tests passed\n";
    return 0;
}
