#include "events/key_code.hpp"
#include "ui/button.hpp"
#include "ui/radio_group.hpp"
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
    ui::Button* a = nullptr;
    ui::Button* b = nullptr;
    ui::Button* c = nullptr;
    std::unique_ptr<ui::UISystem> ui;

    Fixture()
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto first = std::make_unique<ui::Button>("a");
        auto second = std::make_unique<ui::Button>("b");
        auto third = std::make_unique<ui::Button>("c");
        a = first.get();
        b = second.get();
        c = third.get();
        root->add(std::move(first));
        root->add(std::move(second));
        root->add(std::move(third));
        ui = std::make_unique<ui::UISystem>(std::move(root));
        ui->tick();
    }

    ui::Widget* focused() const { return ui->focused(); }

    std::vector<ui::Widget*> walk(std::size_t steps)
    {
        std::vector<ui::Widget*> order;
        for (std::size_t i = 0; i < steps; ++i) {
            ui->focus_next();
            order.push_back(focused());
        }
        return order;
    }
};

void test_default_tree_order()
{
    Fixture f;
    const auto order = f.walk(4);
    assert(order[0] == f.a);
    assert(order[1] == f.b);
    assert(order[2] == f.c);
    assert(order[3] == f.a); // 回绕
}

void test_explicit_order()
{
    Fixture f;
    // 视觉顺序 a, b, c；显式顺序 c(1) → a(2) → b(3)。
    f.c->set_tab_order(1);
    f.a->set_tab_order(2);
    f.b->set_tab_order(3);

    const auto order = f.walk(3);
    assert(order[0] == f.c);
    assert(order[1] == f.a);
    assert(order[2] == f.b);
}

void test_backwards()
{
    Fixture f;
    f.c->set_tab_order(1);
    f.a->set_tab_order(2);
    f.b->set_tab_order(3);

    // 无焦点时 Shift+Tab 从末尾开始。
    f.ui->focus_next(true);
    assert(f.focused() == f.b);

    f.ui->focus_next(true);
    assert(f.focused() == f.a);

    f.ui->focus_next(true);
    assert(f.focused() == f.c);
}

void test_negative_is_skipped()
{
    Fixture f;
    f.b->set_tab_order(-1);

    const auto order = f.walk(3);
    assert(order[0] == f.a);
    assert(order[1] == f.c);
    assert(order[2] == f.a);

    // 程序聚焦仍然允许。
    f.ui->set_focus(f.b);
    assert(f.focused() == f.b);
}

void test_mixed_order()
{
    Fixture f;
    // 只有 b 显式设置：显式 order 排在未设置之前。
    f.b->set_tab_order(5);

    const auto order = f.walk(3);
    assert(order[0] == f.b);
    assert(order[1] == f.a);
    assert(order[2] == f.c);
}

void test_radio_group_roving()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    auto group = std::make_unique<ui::RadioGroup>(200.0f);
    auto* g = group.get();
    g->set_options({"A", "B", "C"}, 0);
    root->add(std::move(group));

    auto next = std::make_unique<ui::Button>("next");
    auto* trailing = next.get();
    root->add(std::move(next));

    ui::UISystem ui{std::move(root)};
    ui.tick();

    // 组只占一个 Tab 停靠点：子 Radio 不参与 Tab。
    assert(g->focusable());
    for (std::size_t i = 0; i < g->option_count(); ++i) {
        assert(g->option(i)->tab_order() == -1);
    }

    // 组获得焦点时，焦点标记落在选中项。
    ui.set_focus(g);
    assert(g->option(0)->is_focused());
    assert(!g->option(1)->is_focused());

    // Down / Up / Home / End 切换选中并带走组内焦点。
    assert(g->on_key_down(event::KeyCode::Down, event::Modifier::None));
    assert(g->selected() == 1);
    assert(!g->option(0)->is_focused());
    assert(g->option(1)->is_focused());

    assert(g->on_key_down(event::KeyCode::Up, event::Modifier::None));
    assert(g->selected() == 0);

    assert(g->on_key_down(event::KeyCode::End, event::Modifier::None));
    assert(g->selected() == 2);
    assert(g->option(2)->is_focused());

    assert(g->on_key_down(event::KeyCode::Home, event::Modifier::None));
    assert(g->selected() == 0);

    // Tab 从组直接跳到下一个控件（不进入组内其它 Radio）。
    ui.set_focus(g);
    ui.focus_next();
    assert(ui.focused() == trailing);

    // 焦点在组内某个 Radio 上（鼠标点击）时，Tab 也跳到组后的控件。
    ui.set_focus(g->option(1));
    ui.focus_next();
    assert(ui.focused() == trailing);

    // 反向同理。
    ui.set_focus(g);
    ui.focus_next(true);
    assert(ui.focused() == trailing);
}

} // namespace

int main()
{
    test_default_tree_order();
    test_explicit_order();
    test_backwards();
    test_negative_is_skipped();
    test_mixed_order();
    test_radio_group_roving();
    std::cout << "ui focus order tests passed\n";
    return 0;
}
