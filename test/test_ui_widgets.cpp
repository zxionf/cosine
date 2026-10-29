#include "ui/core/progress.hpp"
#include "ui/core/radio.hpp"
#include "ui/composites/radio_group.hpp"
#include "ui/root.hpp"
#include "ui/core/tab_bar.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>

int main()
{
    // ---------- TabBar ----------
    {
        xel::ui::TabBar tabs;
        int changes = 0;
        int last = -1;
        tabs.set_on_change([&](int index) { ++changes; last = index; });
        tabs.set_tabs({"One", "Two", "Three"});
        tabs.arrange({{0.0f, 0.0f}, {400.0f, 30.0f}});

        assert(tabs.selected() == 0);
        assert(tabs.measure({400.0f, 30.0f}).y == tabs.height());

        // 点击第二个标签。
        const float second_x = tabs.global_bounds().pos.x + 60.0f;
        assert(tabs.on_click({second_x, 10.0f}, 0));
        assert(tabs.selected() == 1);
        assert(changes >= 1);
        assert(last == 1);

        // 方向键切换，越界被 clamp。
        assert(tabs.on_key_down(xel::event::KeyCode::Right, xel::event::Modifier::None));
        assert(tabs.selected() == 2);
        assert(tabs.on_key_down(xel::event::KeyCode::Right, xel::event::Modifier::None));
        assert(tabs.selected() == 2);
        assert(tabs.on_key_down(xel::event::KeyCode::Left, xel::event::Modifier::None));
        assert(tabs.selected() == 1);
        assert(tabs.on_key_down(xel::event::KeyCode::Home, xel::event::Modifier::None));
        assert(tabs.selected() == 0);
        assert(tabs.on_key_down(xel::event::KeyCode::End, xel::event::Modifier::None));
        assert(tabs.selected() == 2);

        // 点击标签栏外部不改变选中。
        assert(!tabs.on_click({900.0f, 10.0f}, 0));
        assert(tabs.selected() == 2);
    }

    // ---------- Radio / RadioGroup ----------
    {
        xel::ui::RadioGroup group{200.0f};
        int changes = 0;
        int last = -1;
        group.set_on_change([&](int index) { ++changes; last = index; });
        group.set_options({"Alpha", "Beta", "Gamma"}, 0);

        assert(group.option_count() == 3);
        assert(group.selected() == 0);
        assert(group.option(0)->selected());
        assert(!group.option(1)->selected());

        // 点击第二个单选项：组内单选语义。
        assert(group.option(1)->on_click({0.0f, 0.0f}, 0));
        assert(group.selected() == 1);
        assert(!group.option(0)->selected());
        assert(group.option(1)->selected());
        assert(changes == 1);
        assert(last == 1);

        // 直接 select 越界会 clamp。
        group.select(99);
        assert(group.selected() == 2);
        group.select(-5);
        assert(group.selected() == 0);

        // 空格键选中（焦点在 Radio 上）。
        group.select(2);
        assert(group.option(0)->on_key_down(xel::event::KeyCode::Space, xel::event::Modifier::None));
        assert(group.selected() == 0);
    }

    // ---------- Progress ----------
    {
        xel::ui::Progress progress{300.0f};
        assert(progress.value() == 0.0f);
        progress.set_value(0.5f);
        assert(progress.value() == 0.5f);
        progress.set_value(2.0f);
        assert(progress.value() == 1.0f);
        progress.set_value(-1.0f);
        assert(progress.value() == 0.0f);
        progress.arrange({{10.0f, 20.0f}, {300.0f, 12.0f}});
        const auto size = progress.measure({100.0f, 100.0f});
        assert(size.x == 300.0f);
    }

    // ---------- 集成：TabBar 驱动内容切换 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 600.0f, 400.0f);
        auto tabs = std::make_unique<xel::ui::TabBar>();
        auto* tabs_ptr = tabs.get();
        tabs->set_tabs({"A", "B"});
        tabs->set_pos(0.0f, 0.0f);
        root->add(std::move(tabs));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        assert(tabs_ptr->global_bounds().size.x > 0.0f);
        tabs_ptr->set_selected(1);
        assert(tabs_ptr->selected() == 1);
    }

    std::cout << "ui widget tests passed\n";
    return 0;
}
