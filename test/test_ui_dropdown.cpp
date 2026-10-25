#include "ui/dropdown.hpp"
#include "ui/root.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace
{
using xel::event::KeyCode;
using xel::event::Modifier;
} // namespace

int main()
{
    // ---------- 基本状态 ----------
    {
        xel::ui::Dropdown dropdown({"Small", "Medium", "Large"}, 1);
        assert(dropdown.selected() == 1);
        assert(dropdown.selected_text() == "Medium");
        assert(!dropdown.is_open());
        assert(dropdown.measure({200.0f, 30.0f}).y > 0.0f);
    }

    // ---------- 打开 / 键盘选择 / 关闭 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 600.0f, 400.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>(std::vector<std::string>{"A", "B", "C"}, 0);
        auto* dd = dropdown.get();
        int changes = 0;
        int last = -1;
        dd->set_on_change([&](int index) { ++changes; last = index; });
        dd->set_pos(40.0f, 40.0f);
        root->add(std::move(dropdown));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        const std::size_t base = ui.root()->overlays().size();

        dd->open();
        ui.tick();
        assert(dd->is_open());
        assert(ui.root()->overlays().size() == base + 2); // scrim + list
        assert(dd->highlight() == 0);

        assert(dd->on_key_down(KeyCode::Down, Modifier::None));
        assert(dd->highlight() == 1);
        assert(dd->on_key_down(KeyCode::End, Modifier::None));
        assert(dd->highlight() == 2);
        assert(dd->on_key_down(KeyCode::Home, Modifier::None));
        assert(dd->highlight() == 0);
        assert(dd->on_key_down(KeyCode::Up, Modifier::None));
        assert(dd->highlight() == 0); // clamp

        assert(dd->on_key_down(KeyCode::Down, Modifier::None));
        assert(dd->on_key_down(KeyCode::Enter, Modifier::None));
        assert(!dd->is_open());
        assert(dd->selected() == 1);
        assert(changes == 1);
        assert(last == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // Esc 关闭且不改变选择。
        dd->open();
        ui.tick();
        assert(dd->is_open());
        assert(dd->on_key_down(KeyCode::Escape, Modifier::None));
        assert(!dd->is_open());
        assert(dd->selected() == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // Tab 关闭，不消费焦点事件。
        dd->open();
        ui.tick();
        assert(!dd->on_key_down(KeyCode::Tab, Modifier::None));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 点击遮罩关闭。
        dd->open();
        ui.tick();
        assert(ui.root()->overlays().size() == base + 2);
        auto* scrim = ui.root()->overlays()[base].get();
        assert(scrim->on_mouse_down({10.0f, 10.0f}, 0));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 点击列表选项提交。
        dd->open();
        ui.tick();
        assert(ui.root()->overlays().size() == base + 2);
        auto* list = ui.root()->overlays()[base + 1].get();
        const auto bounds = list->global_bounds();
        assert(bounds.size.x >= 160.0f);
        assert(bounds.size.y > 0.0f);
        assert(list->on_click({bounds.pos.x + 4.0f, bounds.pos.y + 4.0f}, 0));
        assert(dd->selected() == 0);
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 列表行悬停高亮。
        dd->open();
        ui.tick();
        list = ui.root()->overlays()[base + 1].get();
        const auto list_bounds = list->global_bounds();
        assert(list->on_mouse_move({list_bounds.pos.x + 4.0f, list_bounds.pos.y + 40.0f}));
        assert(list->on_click({list_bounds.pos.x + 4.0f, list_bounds.pos.y + 40.0f}, 0));
        assert(dd->selected() == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);
    }

    // ---------- 再次点击自身按钮关闭 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>(std::vector<std::string>{"A", "B"}, 0);
        auto* dd = dropdown.get();
        dd->set_pos(20.0f, 20.0f);
        root->add(std::move(dropdown));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();
        const auto base = ui.root()->overlays().size();

        assert(dd->on_mouse_down({30.0f, 30.0f}, 0));
        ui.tick();
        assert(dd->is_open());
        assert(ui.root()->overlays().size() == base + 2);
        assert(dd->on_mouse_down({30.0f, 30.0f}, 0));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);
    }

    // ---------- 空选项不展开 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>();
        auto* dd = dropdown.get();
        root->add(std::move(dropdown));
        xel::ui::UISystem ui{std::move(root)};
        ui.tick();
        const auto before = ui.root()->overlays().size();
        dd->open();
        ui.tick();
        assert(!dd->is_open());
        assert(ui.root()->overlays().size() == before);
    }

    std::cout << "ui dropdown tests passed\n";
    return 0;
}
