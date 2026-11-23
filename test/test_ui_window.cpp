#include "ui/composites/window.hpp"

#include "events/event.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/core/spacer.hpp"
#include "ui/root.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

namespace
{
using xel::ui::Vbox;
using xel::ui::Window;

constexpr float kEps = 1.0e-3f;

struct Fixture
{
    Vbox parent{0.0f, 0.0f, 1200.0f, 800.0f};
    Window* window = nullptr;

    explicit Fixture(glm::vec2 pos = {100.0f, 100.0f}, glm::vec2 size = {320.0f, 200.0f})
    {
        auto w = std::make_unique<Window>("W", pos, size);
        window = w.get();
        parent.add(std::move(w));
        parent.arrange({{0.0f, 0.0f}, {1200.0f, 800.0f}});
    }
};

// 拖动跟手：保留按下时的抓取偏移，而不是位移累加。
void test_drag_follows_cursor()
{
    Fixture f;
    // 在标题栏内按下（相对窗口 60,12）。
    assert(f.window->on_mouse_down({160.0f, 112.0f}, 0));
    assert(f.window->dragging());

    assert(f.window->on_mouse_move({260.0f, 212.0f}));
    assert(std::fabs(f.window->absolute_pos().x - 200.0f) < kEps);
    assert(std::fabs(f.window->absolute_pos().y - 200.0f) < kEps);

    f.window->on_mouse_up({260.0f, 212.0f}, 0);
    assert(!f.window->dragging());
}

// 允许拖出父区域，但至少保留 48px 可见；标题栏不能出顶部。
void test_drag_clamped()
{
    Fixture f;
    f.window->on_mouse_down({160.0f, 112.0f}, 0);
    f.window->on_mouse_move({5000.0f, 5000.0f});
    assert(std::fabs(f.window->absolute_pos().x - (1200.0f - 48.0f)) < kEps);
    assert(std::fabs(f.window->absolute_pos().y - (800.0f - f.window->title_height())) < kEps);

    f.window->on_mouse_move({-5000.0f, -5000.0f});
    // min_x = 48 - 320 = -272；y 不小于 0（标题栏不能出顶部）
    assert(std::fabs(f.window->absolute_pos().x - (-272.0f)) < kEps);
    assert(std::fabs(f.window->absolute_pos().y - 0.0f) < kEps);
    f.window->on_mouse_up({0.0f, 0.0f}, 0);
}

// 点任意位置都置顶：插入序语义（移到 children_ 末尾），不改写 z_index。
void test_raise_on_press()
{
    Vbox parent{0.0f, 0.0f, 1200.0f, 800.0f};
    auto first = std::make_unique<Window>("A", glm::vec2{0.0f, 0.0f}, glm::vec2{200.0f, 150.0f});
    auto second = std::make_unique<Window>("B", glm::vec2{50.0f, 50.0f}, glm::vec2{200.0f, 150.0f});
    Window* a = first.get();
    Window* b = second.get();
    parent.add(std::move(first));
    parent.add(std::move(second));
    parent.arrange({{0.0f, 0.0f}, {1200.0f, 800.0f}});

    // 初始：B 在 A 之后加入 → 画在最上。
    assert(parent.children().back().get() == b);

    b->raise();
    assert(parent.children().back().get() == b); // 已在末尾，不变

    // 点 A 的标题栏 → A 移到末尾（最前）。
    assert(a->on_mouse_down({20.0f, 10.0f}, 0));
    assert(parent.children().back().get() == a);
    // z_index 不被改写。
    assert(a->z_index() == 0);
    assert(b->z_index() == 0);
    a->on_mouse_up({20.0f, 10.0f}, 0);
}

// 右下角同时改宽高；右边只改宽；下边只改高。
void test_resize_grips()
{
    Fixture f;
    // 父容器有 padding：用 global_bounds 算真实边缘，别手写坐标。
    const auto bounds = f.window->global_bounds();
    const float right = bounds.pos.x + bounds.size.x;
    const float bottom = bounds.pos.y + bounds.size.y;

    // 右下角：宽高一起变（抓取偏移 -6，拖动 +100 即尺寸 +100）。
    assert(f.window->on_mouse_down({right - 6.0f, bottom - 6.0f}, 0));
    assert(f.window->resizing());
    f.window->on_mouse_move({right + 94.0f, bottom + 94.0f});
    assert(std::fabs(f.window->width() - 420.0f) < kEps);
    assert(std::fabs(f.window->height() - 300.0f) < kEps);
    f.window->on_mouse_up({right + 94.0f, bottom + 94.0f}, 0);

    // 右边中间：只改宽（先重排一次，让 global_bounds 跟上新的尺寸）。
    f.parent.mark_layout_dirty();
    f.parent.arrange({{0.0f, 0.0f}, {1200.0f, 800.0f}});
    const auto grown = f.window->global_bounds();
    const float edge = grown.pos.x + grown.size.x;
    const float height_before = f.window->height();
    assert(f.window->on_mouse_down({edge - 3.0f, grown.pos.y + 100.0f}, 0));
    f.window->on_mouse_move({edge + 47.0f, grown.pos.y + 100.0f});
    assert(std::fabs(f.window->width() - 470.0f) < kEps);
    assert(std::fabs(f.window->height() - height_before) < kEps);
    f.window->on_mouse_up({edge + 47.0f, grown.pos.y + 100.0f}, 0);
}

// 折叠只留标题栏，展开恢复原高度；关闭两种语义都要能走通。
void test_collapse_and_close()
{
    Fixture f;
    f.window->set_collapsed(true);
    assert(f.window->collapsed());
    assert(std::fabs(f.window->height() - f.window->title_height()) < kEps);
    f.window->set_collapsed(false);
    assert(std::fabs(f.window->height() - 200.0f) < kEps);

    // 关闭叉：右上角（用 global_bounds 算，父容器 padding 会让硬编码坐标偏）。
    bool destroyed = false;
    f.window->set_on_destroy([&destroyed] { destroyed = true; });
    const auto bounds = f.window->global_bounds();
    const glm::vec2 close_point{bounds.pos.x + bounds.size.x - 16.0f, bounds.pos.y + 16.0f};
    assert(f.window->on_mouse_down(close_point, 0));
    // 销毁延迟到下一次布局（避免事件派发期间销毁自身）。
    assert(!destroyed);
    f.window->layout();
    assert(destroyed);

    // 未注册销毁回调时退化为隐藏。
    Fixture g;
    g.window->destroy();
    assert(!g.window->visible());
}
void test_footer_follows_width()
{
    xel::ui::Window win("W", {0.0f, 0.0f}, {800.0f, 400.0f});
    auto footer = std::make_unique<xel::ui::Hbox>(0.0f, 0.0f, 0.0f, 40.0f);
    footer->set_padding(xel::ui::Edges{8.0f, 0.0f, 8.0f, 0.0f});
    footer->set_spacing(8.0f);
    footer->set_min_size({0.0f, 40.0f});
    footer->set_max_size({10000.0f, 40.0f});
    footer->set_cross_align(xel::ui::Align::center_left());
    footer->add(std::make_unique<xel::ui::Label>("framework", 16.0f));
    auto spacer = std::make_unique<xel::ui::Spacer>(1.0f, 1.0f);
    spacer->set_flex_grow(1.0f);
    footer->add(std::move(spacer));
    auto import = std::make_unique<xel::ui::Button>("Import", [] {});
    import->set_min_size({0.0f, 28.0f});
    import->set_max_size({1000.0f, 28.0f});
    auto* button = import.get();
    footer->add(std::move(import));
    win.set_footer(std::move(footer));

    win.set_size({900.0f, 400.0f});
    win.arrange({{0.0f, 0.0f}, {900.0f, 400.0f}});
    const float wide = button->global_bounds().pos.x;
    win.set_size({820.0f, 400.0f});
    win.arrange({{0.0f, 0.0f}, {820.0f, 400.0f}});
    const float narrow = button->global_bounds().pos.x;
    assert(narrow < wide);
    assert(std::fabs((wide - narrow) - 80.0f) < kEps);

    win.set_size({900.0f, 400.0f});
    win.arrange({{0.0f, 0.0f}, {900.0f, 400.0f}});
    assert(std::fabs(button->global_bounds().pos.x - wide) < kEps);
}

// 命中裁剪与绘制裁剪一致：布局到窗口外（画不出来）的子项不可点。
void test_clips_children()
{
    Fixture f;
    assert(f.window->clips_children());

    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1200.0f, 800.0f);
    xel::ui::UISystem ui{std::move(root)};

    int clicks = 0;
    auto win = std::make_unique<Window>("W", glm::vec2{100.0f, 100.0f}, glm::vec2{320.0f, 200.0f});
    auto btn = std::make_unique<xel::ui::Button>("out", [&clicks] { ++clicks; });
    btn->set_absolute(true);
    btn->set_absolute_pos({300.0f, 40.0f});
    btn->set_min_size({120.0f, 40.0f});
    xel::ui::Button* btn_ptr = btn.get();
    win->add(std::move(btn));
    ui.root()->add(std::move(win));
    ui.tick();

    // 子项确实超出了窗口右边界（画不出来）。
    const auto wb = ui.root()->children().front()->global_bounds();
    const auto bb = btn_ptr->global_bounds();
    assert(bb.pos.x + bb.size.x > wb.pos.x + wb.size.x);

    // 点击窗口内部分：命中。
    const glm::vec2 in_window{bb.pos.x + 4.0f, bb.pos.y + bb.size.y * 0.5f};
    assert(wb.contains(in_window));
    xel::event::InputEvent in_down;
    in_down.type = xel::event::InputEvent::Type::MouseButtonDown;
    in_down.mouse_pos = in_window;
    ui.dispatch(in_down);
    xel::event::InputEvent in_up;
    in_up.type = xel::event::InputEvent::Type::MouseButtonUp;
    in_up.mouse_pos = in_window;
    ui.dispatch(in_up);
    assert(clicks == 1);

    // 点击窗口外（子项溢出区域）：不命中子项。
    const glm::vec2 outside{wb.pos.x + wb.size.x + 20.0f, bb.pos.y + bb.size.y * 0.5f};
    const int before = clicks;
    xel::event::InputEvent down;
    down.type = xel::event::InputEvent::Type::MouseButtonDown;
    down.mouse_pos = outside;
    ui.dispatch(down);
    xel::event::InputEvent up;
    up.type = xel::event::InputEvent::Type::MouseButtonUp;
    up.mouse_pos = outside;
    ui.dispatch(up);
    assert(clicks == before);
}

// 折叠态：关闭叉不画也不命中，只能先展开。
void test_collapsed_close_disabled()
{
    Fixture f;
    f.window->set_collapsed(true);
    assert(f.window->collapsed());

    const auto bounds = f.window->global_bounds();
    const glm::vec2 close_point{bounds.pos.x + bounds.size.x - 16.0f, bounds.pos.y + 16.0f};
    f.window->on_mouse_down(close_point, 0);
    // 折叠态下关闭被禁用：窗口仍在、仍折叠（既没销毁也没隐藏）。
    assert(f.window->collapsed());
    assert(f.window->visible());
}

// 模态结束后 always_on_top 还原，不再永久置顶。
void test_always_on_top_not_sticky_after_modal()
{
    Fixture f;
    assert(!f.window->always_on_top());

    f.window->set_modal(true);
    assert(f.window->always_on_top()); // 模态期间临时置顶
    f.window->set_modal(false);
    assert(!f.window->always_on_top()); // 结束后还原
    assert(f.window->z_index() != 10000);

    // 本来就置顶的窗口，模态结束后仍保持置顶。
    Fixture g;
    g.window->set_always_on_top(true);
    g.window->set_modal(true);
    g.window->set_modal(false);
    assert(g.window->always_on_top());
}

// Esc 语义：默认不关窗口；on_close 拒绝时不吞事件。
void test_esc_and_close_semantics()
{
    Fixture f;
    // 默认 esc_closable = false：普通窗口的 Esc 不关窗口。
    assert(!f.window->esc_closable());
    assert(!f.window->on_key_down(xel::event::KeyCode::Escape, xel::event::Modifier::None));
    assert(f.window->visible());

    // 显式开启后，on_close 拒绝 → request_close 返回 false，窗口保留。
    f.window->set_esc_closable(true);
    f.window->set_on_close([] { return false; });
    assert(!f.window->request_close());
    assert(!f.window->on_key_down(xel::event::KeyCode::Escape, xel::event::Modifier::None));
    assert(f.window->visible());

    // 同意关闭 → 返回 true，销毁延迟到 layout。
    bool destroyed = false;
    f.window->set_on_destroy([&destroyed] { destroyed = true; });
    f.window->set_on_close([] { return true; });
    assert(f.window->on_key_down(xel::event::KeyCode::Escape, xel::event::Modifier::None));
    assert(!destroyed);
    f.window->layout();
    assert(destroyed);
}
// 标题栏高度 = 字体 line box 高度（不额外加 padding）。
void test_title_follows_line_height()
{
    Fixture f;
    const auto& t = f.window->ui_theme();
    auto* atlas = f.window->font_atlas();
    const float line = atlas != nullptr ? atlas->line_height(t.text_size) : t.text_size * 1.2f;
    assert(std::fabs(f.window->title_height() - line) < kEps);

    // chrome 的三个派生几何一致：glyph = icon + 2*inset，且图标不超出标题栏。
    const auto c = f.window->chrome();
    assert(std::fabs(c.glyph - (c.icon + c.inset * 2.0f)) < kEps);
    assert(c.icon + c.inset * 2.0f <= c.title + kEps);

    // 无标题栏时高度为 0。
    f.window->set_title_bar_visible(false);
    assert(std::fabs(f.window->title_height()) < kEps);

    // 真正“跟随”：主题字号变大 → 标题行高变大 → 标题高度变大。
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
    xel::ui::UISystem ui{std::move(root)};
    auto win = std::make_unique<Window>("W", glm::vec2{0.0f, 0.0f}, glm::vec2{300.0f, 200.0f});
    Window* w = win.get();
    ui.root()->add(std::move(win));
    ui.tick();
    const float small = w->title_height();

    auto bigger = ui.root()->ui_theme();
    bigger.text_size = 28.0f;
    ui.root()->set_theme(bigger);
    ui.tick();
    const float large = w->title_height();
    assert(large > small);
}

// 关闭叉与折叠三角的命中矩形位于标题栏内、不重叠且各自可命中。
void test_chrome_hit_rects_consistent()
{
    Fixture f;
    const auto bounds = f.window->global_bounds();
    const auto c = f.window->chrome();
    const float title = f.window->title_height();

    // 折叠三角（左）：点中心应折叠。
    const glm::vec2 collapse_center{bounds.pos.x + c.inset + c.icon * 0.5f,
                                    bounds.pos.y + c.inset + c.icon * 0.5f};
    f.window->on_mouse_down(collapse_center, 0);
    assert(f.window->collapsed());
    f.window->on_mouse_up(collapse_center, 0);
    f.window->set_collapsed(false);

    // 关闭叉（右）：点中心应请求关闭（延迟到 layout）。
    bool destroyed = false;
    f.window->set_on_destroy([&destroyed] { destroyed = true; });
    const glm::vec2 close_center{bounds.pos.x + bounds.size.x - c.inset - c.icon * 0.5f,
                                 bounds.pos.y + c.inset + c.icon * 0.5f};
    assert(f.window->on_mouse_down(close_center, 0));
    f.window->layout();
    assert(destroyed);
    (void)title;
}
// 窗口是 fixed_size 控件：放进 Vbox 也不会被拉伸成 slot 尺寸。
void test_fixed_size_not_stretched()
{
    Vbox parent{0.0f, 0.0f, 1200.0f, 800.0f};
    auto w = std::make_unique<Window>("W", glm::vec2{50.0f, 50.0f}, glm::vec2{300.0f, 200.0f});
    Window* win = w.get();
    assert(win->fixed_size());
    parent.add(std::move(w));

    // 给一个远大于窗口的 slot：尺寸应保持 300x200。
    win->arrange({{0.0f, 0.0f}, {1000.0f, 700.0f}});
    assert(std::fabs(win->width() - 300.0f) < kEps);
    assert(std::fabs(win->height() - 200.0f) < kEps);
    assert(std::fabs(win->global_bounds().size.x - 300.0f) < kEps);
}

} // namespace

int main()
{
    test_drag_follows_cursor();
    test_drag_clamped();
    test_raise_on_press();
    test_resize_grips();
    test_collapse_and_close();
    test_footer_follows_width();
    test_clips_children();
    test_collapsed_close_disabled();
    test_always_on_top_not_sticky_after_modal();
    test_esc_and_close_semantics();
    test_title_follows_line_height();
    test_chrome_hit_rects_consistent();
    test_fixed_size_not_stretched();
    std::printf("test_ui_window: all passed\n");
    return 0;
}
