#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/tab_view.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
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

glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.global_bounds();
    return {bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f};
}

std::unique_ptr<xel::ui::Widget> page_with_label(const std::string& text)
{
    auto panel = std::make_unique<xel::ui::Panel>(0.0f, 0.0f, 300.0f, 120.0f);
    panel->add(std::make_unique<xel::ui::Label>(text));
    return panel;
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);

    auto view = std::make_unique<xel::ui::TabView>();
    view->set_pos(20.0f, 20.0f);
    view->set_min_size({360.0f, 220.0f});

    int changes = 0;
    int last_change = -1;
    view->set_on_change([&](int index) {
        ++changes;
        last_change = index;
    });

    assert(view->add_page("First", page_with_label("first page")) == 0);
    assert(view->add_page("Second", page_with_label("second page")) == 1);
    assert(view->add_page("Third", page_with_label("third page")) == 2);
    assert(view->page_count() == 3);

    // 初始选中第一页：只有它可见。
    assert(view->selected() == 0);
    assert(view->page(0)->visible());
    assert(!view->page(1)->visible());
    assert(!view->page(2)->visible());

    xel::ui::TabView* view_ptr = view.get();
    root->add(std::move(view));

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    // children 暴露给命中测试：标签栏 + 3 个页面。
    assert(ui.root()->children().size() == 1);
    assert(view_ptr->children().size() == 4);
    assert(ui.root()->children().front().get() == view_ptr);

    // 标签栏高度进入 TabView 的测量结果。
    const auto measured = view_ptr->measure({400.0f, 300.0f});
    assert(measured.y > view_ptr->tab_bar().measure({400.0f, 300.0f}).y);
    assert(view_ptr->global_bounds().size.x >= 360.0f);

    // 点击第二个标签切换页面。
    const auto& tabs = view_ptr->tab_bar();
    assert(tabs.tabs().size() == 3);
    // 第二个标签中心：标签栏起点 + 第一个标签宽度 + spacing + 半个标签宽度。
    const auto bar_bounds = tabs.global_bounds();
    glm::vec2 click{bar_bounds.pos.x + 8.0f, bar_bounds.pos.y + bar_bounds.size.y * 0.5f};
    // 逐像素向右找第二段标签的可点击位置。
    bool clicked = false;
    for (float dx = 0.0f; dx < bar_bounds.size.x; dx += 2.0f) {
        const glm::vec2 p{bar_bounds.pos.x + dx, click.y};
        ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, p));
        ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, p));
        if (view_ptr->selected() != 0) {
            clicked = true;
            break;
        }
    }
    assert(clicked);
    assert(view_ptr->selected() == 1);
    assert(changes == 1);
    assert(last_change == 1);
    assert(!view_ptr->page(0)->visible());
    assert(view_ptr->page(1)->visible());
    assert(!view_ptr->page(2)->visible());

    // 键盘切换：点击标签栏使其获得焦点后 Right/Left/Home/End。
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, center(view_ptr->tab_bar())));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, center(view_ptr->tab_bar())));

    ui.dispatch([] {
        InputEvent event;
        event.type = InputEvent::Type::KeyDown;
        event.key = KeyCode::Right;
        return event;
    }());
    assert(view_ptr->selected() == 2);
    assert(view_ptr->page(2)->visible());

    ui.dispatch([] {
        InputEvent event;
        event.type = InputEvent::Type::KeyDown;
        event.key = KeyCode::Home;
        return event;
    }());
    assert(view_ptr->selected() == 0);

    ui.dispatch([] {
        InputEvent event;
        event.type = InputEvent::Type::KeyDown;
        event.key = KeyCode::End;
        return event;
    }());
    assert(view_ptr->selected() == 2);

    // set_selected 直接切换并通知。
    const int before = changes;
    view_ptr->set_selected(1);
    assert(view_ptr->selected() == 1);
    assert(changes == before + 1);
    assert(view_ptr->page(1)->visible());
    assert(!view_ptr->page(2)->visible());

    // 越界被 clamp。
    view_ptr->set_selected(99);
    assert(view_ptr->selected() == 2);
    view_ptr->set_selected(-5);
    assert(view_ptr->selected() == 0);

    // set_page 替换页面内容并沿用当前可见性。
    view_ptr->set_page(0, page_with_label("replaced"));
    assert(view_ptr->page(0) != nullptr);
    assert(view_ptr->page(0)->visible());

    std::cout << "tab view tests passed\n";
    return 0;
}
