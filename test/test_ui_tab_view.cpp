#include "ui/core/color_rect.hpp"
#include "ui/core/label.hpp"
#include "ui/containers/panel.hpp"
#include "ui/composites/tab_view.hpp"
#include "ui/system/ui_system.hpp"

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

glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.bounds();
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
    assert(view_ptr->bounds().size.x >= 360.0f);

    // 点击第二个标签切换页面。
    const auto& tabs = view_ptr->tab_bar();
    assert(tabs.tabs().size() == 3);
    // 第二个标签中心：标签栏起点 + 第一个标签宽度 + spacing + 半个标签宽度。
    const auto bar_bounds = tabs.bounds();
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

    // 竖向 TabBar + 左侧栏：measure / arrange / 命中 / 键盘。
    auto side = std::make_unique<xel::ui::TabView>();
    side->set_bar_position(xel::ui::TabView::BarPosition::Left);
    side->set_pos(400.0f, 20.0f);
    side->set_min_size({300.0f, 220.0f});
    side->add_page("Alpha", page_with_label("alpha"));
    side->add_page("Beta", page_with_label("beta"));
    auto* side_ptr = side.get();
    ui.root()->add(std::move(side));
    ui.tick();

    auto& side_bar = side_ptr->tab_bar();
    assert(side_bar.orientation() == xel::ui::TabBar::Orientation::Vertical);
    const float control_h = ui.context()->theme().control_height;
    const float spacing = ui.context()->theme().spacing;
    assert(side_bar.measure({300.0f, 300.0f}).y >= control_h * 2.0f);
    assert(side_bar.bounds().size.x < side_ptr->bounds().size.x);
    assert(side_bar.bounds().size.y >= control_h * 2.0f);
    assert(side_ptr->page(0)->bounds().pos.x > side_bar.bounds().pos.x);

    // 第二项位于竖条下半部分；点击切换。
    const auto side_bar_bounds = side_bar.bounds();
    const glm::vec2 second{side_bar_bounds.pos.x + 8.0f,
                           side_bar_bounds.pos.y + control_h + spacing + control_h * 0.5f};
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, second));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, second));
    assert(side_ptr->selected() == 1);
    assert(side_ptr->page(1)->visible());
    assert(!side_ptr->page(0)->visible());

    // 竖向键盘导航用 Up/Down。
    ui.set_focus(&side_bar);
    ui.dispatch([] {
        InputEvent event;
        event.type = InputEvent::Type::KeyDown;
        event.key = KeyCode::Up;
        return event;
    }());
    assert(side_ptr->selected() == 0);

    // set_tab_gap(0) + set_padding(0)：页面紧贴标签栏，且窗口内无内缩（CO-7）。
    auto tight = std::make_unique<xel::ui::TabView>();
    tight->set_padding(0.0f);
    tight->set_tab_gap(0.0f);
    tight->add_page("Tight", page_with_label("tight"));
    auto* tight_ptr = tight.get();
    ui.root()->add(std::move(tight));
    ui.tick();

    assert(tight_ptr->tab_gap() == 0.0f);
    assert(tight_ptr->padding().left == 0.0f);
    // 主题重刷（set_theme 会全树 set_context）后显式值不被覆盖。
    {
        xel::ui::Theme t = ui.context()->theme();
        t.padding = 20.0f;
        ui.root()->set_theme(t);
    }
    assert(tight_ptr->padding().left == 0.0f);

    const auto tight_bounds = tight_ptr->bounds();
    const auto tight_bar = tight_ptr->tab_bar().bounds();
    const auto tight_page = tight_ptr->page(0)->bounds();
    assert(std::fabs(tight_bar.pos.x - tight_bounds.pos.x) < 0.01f);
    assert(std::fabs(tight_bar.pos.y - tight_bounds.pos.y) < 0.01f);
    assert(std::fabs(tight_page.pos.x - tight_bounds.pos.x) < 0.01f);
    // 页面顶边 = 标签栏底边（不再插 theme.spacing 的缝）。
    assert(std::fabs(tight_page.pos.y - (tight_bar.pos.y + tight_bar.size.y)) < 0.01f);

    // UI-33：measure 只算当前页（page_fill=false 时按页面自然尺寸）。
    {
        auto sized = std::make_unique<xel::ui::TabView>();
        sized->set_page_fill(false);
        sized->add_page("Small", std::make_unique<xel::ui::ColorRect>(0.0f, 0.0f, 120.0f, 60.0f, glm::vec4{1.0f}));
        sized->add_page("Large", std::make_unique<xel::ui::ColorRect>(0.0f, 0.0f, 300.0f, 200.0f, glm::vec4{1.0f}));
        auto* sized_ptr = sized.get();
        ui.root()->add(std::move(sized));
        ui.tick();

        const glm::vec2 small = sized_ptr->measure({600.0f, 400.0f});
        sized_ptr->set_selected(1);
        const glm::vec2 large = sized_ptr->measure({600.0f, 400.0f});
        // 不测非当前页：切到大页后 measure 变大；反之不因隐藏大页而虚高。
        assert(large.x > small.x + 100.0f);
        assert(large.y > small.y + 100.0f);
        // page_fill=true（默认）下页面吃满内容区。
        auto fill = std::make_unique<xel::ui::TabView>();
        fill->add_page("F", std::make_unique<xel::ui::ColorRect>(0.0f, 0.0f, 10.0f, 10.0f, glm::vec4{1.0f}));
        auto* fill_ptr = fill.get();
        ui.root()->add(std::move(fill));
        ui.tick();
        const glm::vec2 filled = fill_ptr->measure({500.0f, 300.0f});
        assert(filled.x >= 500.0f - 0.01f);
        assert(filled.y >= 300.0f - 0.01f);
    }

    std::cout << "tab view tests passed\n";
    return 0;
}
