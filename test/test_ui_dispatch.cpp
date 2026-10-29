// 阶段 1 安全网：走真实派发路径（UIRoot + UISystem），覆盖 hit_path + 冒泡 + 布局。
// 与 test_ui_window 互补：那边直接调 on_mouse_*，绕过了派发链。
#include "events/event.hpp"
#include "graphics/drawable.hpp"
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/root.hpp"
#include "ui/composites/section.hpp"
#include "ui/ui_system.hpp"
#include "ui/composites/window.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace xel;

namespace
{
constexpr int kLeft = 0;

struct Fixture
{
    std::unique_ptr<ui::UISystem> system;
    ui::UIRoot* root = nullptr;

    Fixture()
    {
        auto r = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
        root = r.get();
        system = std::make_unique<ui::UISystem>(std::move(r));
        system->tick();
    }

    void add(std::unique_ptr<ui::Widget> widget) { root->add(std::move(widget)); }
    void layout() { system->tick(); }

    void move(glm::vec2 pos)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseMove;
        e.mouse_pos = pos;
        system->dispatch(e);
    }
    void down(glm::vec2 pos, int button = kLeft)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseButtonDown;
        e.mouse_pos = pos;
        e.mouse_button = button;
        system->dispatch(e);
    }
    void up(glm::vec2 pos, int button = kLeft)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseButtonUp;
        e.mouse_pos = pos;
        e.mouse_button = button;
        system->dispatch(e);
    }
    void click(glm::vec2 pos, int button = kLeft)
    {
        down(pos, button);
        up(pos, button);
    }
};

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.01f;
}

// 1) 派发链能到叶子控件。
void test_click_button()
{
    Fixture f;
    int clicks = 0;
    auto button = std::make_unique<ui::Button>("ok", [&clicks] { ++clicks; });
    button->set_pos(20.0f, 20.0f);
    button->set_size(100.0f, 40.0f);
    f.add(std::move(button));
    f.layout();

    f.click({50.0f, 40.0f});
    assert(clicks == 1);
    std::printf("click button ok\n");
}

// 2) 标题栏拖动：hit_path 末端就是 Window 自己。
void test_drag_window_by_title()
{
    Fixture f;
    auto window = std::make_unique<ui::Window>("w", glm::vec2{100.0f, 100.0f},
                                               glm::vec2{300.0f, 200.0f});
    ui::Window* win = window.get();
    f.add(std::move(window));
    f.layout();
    assert(near(win->absolute_pos().x, 100.0f));

    f.down({160.0f, 112.0f});
    f.move({200.0f, 142.0f});
    assert(near(win->absolute_pos().x, 140.0f));
    assert(near(win->absolute_pos().y, 130.0f));
    f.up({200.0f, 142.0f});
    std::printf("drag by title ok\n");
}

// 3) 无标题栏窗口：内容区空白处拖动，必须能从 Vbox 冒泡回 Window。
//    这是「body_ 缺 set_parent」那个 bug 的回归用例（有标题栏的窗口故意只支持拖标题栏）。
void test_drag_window_by_content()
{
    Fixture f;
    auto window = std::make_unique<ui::Window>("w", glm::vec2{100.0f, 100.0f},
                                               glm::vec2{300.0f, 200.0f});
    window->set_title_bar_visible(false);
    ui::Window* win = window.get();
    f.add(std::move(window));
    f.layout();

    f.down({160.0f, 200.0f}); // 内容区（标题栏之下）
    f.move({200.0f, 230.0f});
    if (!near(win->absolute_pos().x, 140.0f)) {
        std::fprintf(stderr, "content drag x = %f\n", win->absolute_pos().x);
    }
    assert(near(win->absolute_pos().x, 140.0f));
    assert(near(win->absolute_pos().y, 130.0f));
    f.up({200.0f, 230.0f});
    std::printf("drag by content ok\n");
}

// 4) 模态期间窗口外的点击被遮罩吃掉。
void test_modal_blocks_outside()
{
    Fixture f;
    int clicks = 0;
    auto button = std::make_unique<ui::Button>("ok", [&clicks] { ++clicks; });
    button->set_pos(20.0f, 20.0f);
    button->set_size(100.0f, 40.0f);
    f.add(std::move(button));

    auto window = std::make_unique<ui::Window>("w", glm::vec2{400.0f, 300.0f},
                                               glm::vec2{300.0f, 200.0f});
    ui::Window* win = window.get();
    f.add(std::move(window));
    f.layout();

    f.click({50.0f, 40.0f});
    assert(clicks == 1);

    win->set_modal(true);
    f.layout();
    f.click({50.0f, 40.0f});
    assert(clicks == 1); // 被遮罩挡住

    win->set_modal(false);
    f.layout();
    f.click({50.0f, 40.0f});
    if (clicks != 2) {
        std::fprintf(stderr, "after unblock: clicks = %d (expected 2), root children = %zu\n",
                     clicks, f.root->children().size());
    }
    assert(clicks == 2);
    assert(f.root->children().size() == 2); // button + window，遮罩已移除
    std::printf("modal blocks outside ok\n");
}

// 5) 内容重建后，显式布局能把新子项排出去（当前契约：需一次 layout）。
void test_rebuild_relayouts()
{
    Fixture f;
    auto section = std::make_unique<ui::Section>("S");
    ui::Section* sec = section.get();
    sec->set_pos(10.0f, 10.0f);
    sec->set_size(240.0f, 300.0f);
    f.add(std::move(section));
    f.layout();

    sec->add(std::make_unique<ui::Label>("first", 18.0f));
    f.layout();
    const std::size_t before = sec->children().size();
    assert(before == 1);

    sec->clear();
    sec->add(std::make_unique<ui::Label>("second", 18.0f));
    f.layout();

    assert(sec->children().size() == 1);
    const ui::Widget* row = sec->children().front().get();
    if (!(row->global_bounds().size.y > 0.0f)) {
        std::fprintf(stderr, "rebuilt row height = %f\n", row->global_bounds().size.y);
    }
    assert(row->global_bounds().size.y > 0.0f);
    std::printf("rebuild relayouts ok\n");
}
} // namespace

int main()
{
    test_click_button();
    test_drag_window_by_title();
    test_drag_window_by_content();
    test_modal_blocks_outside();
    test_rebuild_relayouts();
    std::printf("ui dispatch tests passed\n");
    return 0;
}
