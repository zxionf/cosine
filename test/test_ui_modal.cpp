// 模态键盘与焦点隔离测试：按键/字符不穿透、初始焦点、关闭恢复、嵌套模态、
// 对话框作用域、遮罩手势吞掉、悬空模态清理。
#include "events/event.hpp"
#include "ui/composites/dialog.hpp"
#include "ui/composites/window.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/scrim.hpp"
#include "ui/system/root.hpp"
#include "ui/system/ui_system.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

using namespace xel;

namespace
{
// 记录键盘/字符事件的焦点控件。
struct KeyCounter : ui::Widget
{
    int key_down = 0;
    int chars = 0;

    bool focusable() const override { return true; }
    bool hit_testable() const override { return true; }
    glm::vec2 measure(glm::vec2) override { return {60.0f, 40.0f}; }
    void arrange(ui::Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList&) const override {}
    bool on_key_down(event::KeyCode, event::Modifier) override { ++key_down; return true; }
    bool on_char(char32_t) override { ++chars; return true; }
};

// 记录各类指针手势的命中控件。
struct GestureCounter : ui::Widget
{
    int clicks = 0;
    int right_clicks = 0;
    int double_clicks = 0;
    int long_presses = 0;

    bool focusable() const override { return true; }
    glm::vec2 measure(glm::vec2) override { return {100.0f, 100.0f}; }
    void arrange(ui::Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList&) const override {}
    bool on_click(glm::vec2, int) override { ++clicks; return true; }
    bool on_right_click(glm::vec2) override { ++right_clicks; return true; }
    bool on_double_click(glm::vec2, int) override { ++double_clicks; return true; }
    bool on_long_press(glm::vec2, int) override { ++long_presses; return true; }
};

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
    void tick(float dt = 1.0f / 60.0f) { system->tick(dt); }
    ui::Widget* focused() const { return system->focused(); }

    void key_down(event::KeyCode code)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::KeyDown;
        e.key = code;
        e.mods = event::Modifier::None;
        system->dispatch(e);
    }
    void char_input(char32_t cp)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::CharInput;
        e.codepoint = cp;
        system->dispatch(e);
    }
    void down(glm::vec2 pos, int button = 0)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseButtonDown;
        e.mouse_pos = pos;
        e.mouse_button = button;
        system->dispatch(e);
    }
    void up(glm::vec2 pos, int button = 0)
    {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseButtonUp;
        e.mouse_pos = pos;
        e.mouse_button = button;
        system->dispatch(e);
    }
    void click(glm::vec2 pos, int button = 0)
    {
        down(pos, button);
        up(pos, button);
    }
};

// 1) 模态打开时，焦点在作用域外的按键/字符被忽略（不穿透到被遮罩控件）。
void test_key_char_blocked_outside_modal()
{
    Fixture f;
    auto outer = std::make_unique<KeyCounter>();
    KeyCounter* outer_ptr = outer.get();
    outer->set_pos(10.0f, 10.0f);
    f.add(std::move(outer));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{300.0f, 200.0f}, glm::vec2{200.0f, 150.0f});
    ui::Window* win_ptr = win.get();
    win->add(std::make_unique<ui::Button>("ok", [] {}));
    f.add(std::move(win));
    f.tick();

    f.system->set_focus(outer_ptr);
    win_ptr->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == win_ptr);

    // 模拟焦点仍在作用域外（被遮罩控件）：按键与字符都应被忽略。
    f.system->set_focus(outer_ptr);
    f.key_down(event::KeyCode::A);
    f.char_input(U'x');
    assert(outer_ptr->key_down == 0);
    assert(outer_ptr->chars == 0);
    std::printf("key/char blocked outside modal ok\n");
}

// 2) 打开模态时焦点自动移到模态内首个可聚焦控件。
void test_initial_focus_moves_into_modal()
{
    Fixture f;
    auto outer = std::make_unique<KeyCounter>();
    KeyCounter* outer_ptr = outer.get();
    f.add(std::move(outer));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{300.0f, 200.0f}, glm::vec2{200.0f, 150.0f});
    ui::Window* win_ptr = win.get();
    auto ok = std::make_unique<ui::Button>("ok", [] {});
    ui::Button* ok_ptr = ok.get();
    win->add(std::move(ok));
    f.add(std::move(win));
    f.tick();

    f.system->set_focus(outer_ptr);
    win_ptr->set_modal(true);
    f.tick();
    assert(f.focused() == ok_ptr); // 模态内首个可聚焦按钮
    std::printf("initial focus moves into modal ok\n");
}

// 3) 关闭模态后焦点恢复到打开前的控件。
void test_focus_restored_on_close()
{
    Fixture f;
    auto outer = std::make_unique<KeyCounter>();
    KeyCounter* outer_ptr = outer.get();
    f.add(std::move(outer));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{300.0f, 200.0f}, glm::vec2{200.0f, 150.0f});
    ui::Window* win_ptr = win.get();
    win->add(std::make_unique<ui::Button>("ok", [] {}));
    f.add(std::move(win));
    f.tick();

    f.system->set_focus(outer_ptr);
    win_ptr->set_modal(true);
    f.tick();
    assert(f.focused() != outer_ptr); // 焦点已移入模态

    win_ptr->set_modal(false);
    f.tick();
    assert(f.focused() == outer_ptr); // 恢复
    std::printf("focus restored on close ok\n");
}

// 4) 嵌套模态：逐层弹出时焦点按快照逐层恢复。
void test_nested_modal_focus_restore()
{
    Fixture f;
    auto outer = std::make_unique<KeyCounter>();
    KeyCounter* outer_ptr = outer.get();
    f.add(std::move(outer));

    auto w1 = std::make_unique<ui::Window>("A", glm::vec2{100.0f, 100.0f}, glm::vec2{200.0f, 150.0f});
    auto b1 = std::make_unique<ui::Button>("a", [] {});
    ui::Button* b1_ptr = b1.get();
    w1->add(std::move(b1));
    ui::Window* w1_ptr = w1.get();
    f.add(std::move(w1));

    auto w2 = std::make_unique<ui::Window>("B", glm::vec2{150.0f, 150.0f}, glm::vec2{200.0f, 150.0f});
    auto b2 = std::make_unique<ui::Button>("b", [] {});
    ui::Button* b2_ptr = b2.get();
    w2->add(std::move(b2));
    ui::Window* w2_ptr = w2.get();
    f.add(std::move(w2));
    f.tick();

    f.system->set_focus(outer_ptr);
    w1_ptr->set_modal(true);
    f.tick();
    assert(f.focused() == b1_ptr);

    w2_ptr->set_modal(true);
    f.tick();
    assert(f.focused() == b2_ptr);
    assert(f.root->modal_top() == w2_ptr);

    w2_ptr->set_modal(false);
    f.tick();
    assert(f.focused() == b1_ptr); // 弹回 A 的焦点快照

    w1_ptr->set_modal(false);
    f.tick();
    assert(f.focused() == outer_ptr);
    std::printf("nested modal focus restore ok\n");
}

// 5) 对话框（overlay 宿主）纳入模态作用域：初始焦点落到主按钮，且屏蔽作用域外按键。
void test_dialog_modal_scope()
{
    Fixture f;
    auto outer = std::make_unique<KeyCounter>();
    KeyCounter* outer_ptr = outer.get();
    f.add(std::move(outer));

    auto dialog = std::make_unique<ui::Dialog>("Confirm", 360.0f);
    auto* ok = dialog->add_action("OK", [] {}, /*primary=*/true);
    auto* host = ui::show_dialog(*f.root, std::move(dialog));
    f.tick();

    assert(f.root->modal_top() == host);
    assert(f.focused() == ok); // 对话框主按钮

    // 作用域外焦点被忽略。
    f.system->set_focus(outer_ptr);
    f.key_down(event::KeyCode::A);
    assert(outer_ptr->key_down == 0);

    host->close();
    f.tick();
    assert(f.root->modal_top() == nullptr);
    std::printf("dialog modal scope ok\n");
}

// 5b) 模态窗口内弹对话框：DialogHost 是 Modal 层，不被窗口 Scrim 盖住。
void test_dialog_over_modal_window()
{
    Fixture f;
    auto win = std::make_unique<ui::Window>("W", glm::vec2{100.0f, 100.0f}, glm::vec2{300.0f, 200.0f});
    ui::Window* win_ptr = win.get();
    win->add(std::make_unique<ui::Button>("w-ok", [] {}));
    f.add(std::move(win));
    f.tick();
    win_ptr->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == win_ptr);

    int ok_calls = 0;
    auto dialog = std::make_unique<ui::Dialog>("Confirm", 360.0f);
    auto* ok = dialog->add_action("OK", [&ok_calls] { ++ok_calls; }, /*primary=*/true);
    auto* host = ui::show_dialog(*f.root, std::move(dialog));
    f.tick();

    assert(host->layer() == ui::Widget::Layer::Modal);
    assert(f.root->modal_top() == host); // 对话框成为最顶层模态

    // 对话框内按钮可命中（不被窗口 Scrim 盖住）。
    const auto bounds = ok->global_bounds();
    f.click({bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f});
    assert(ok_calls == 1);

    // 关闭对话框后回到窗口。
    host->close();
    f.tick();
    assert(f.root->modal_top() == win_ptr);
    std::printf("dialog over modal window ok\n");
}

// 5c) 模态栈不变量：scrim 配对（with_scrim 的帧才加 scrim）、push/pop/悬空剔除后栈一致。
void test_modal_frame_invariants()
{
    Fixture f;
    auto w1 = std::make_unique<ui::Window>("A", glm::vec2{50.0f, 50.0f}, glm::vec2{200.0f, 150.0f});
    ui::Window* w1_ptr = w1.get();
    f.add(std::move(w1));
    f.tick();
    const std::size_t base_children = f.root->children().size();

    // with_scrim=true：多一个 Scrim 进 children。
    w1_ptr->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == w1_ptr);
    assert(f.root->children().size() == base_children + 1);

    // with_scrim=false（对话框）：不加 Scrim，children 不变。
    auto dialog = std::make_unique<ui::Dialog>("Confirm", 300.0f);
    dialog->add_action("OK", [] {}, true);
    auto* host = ui::show_dialog(*f.root, std::move(dialog));
    f.tick();
    assert(f.root->modal_top() == host);
    assert(f.root->children().size() == base_children + 1);

    // pop 对话框：窗口的 Scrim 仍在。
    host->close();
    f.tick();
    assert(f.root->modal_top() == w1_ptr);
    assert(f.root->children().size() == base_children + 1);

    // pop 窗口：Scrim 被移除，children 回到基线。
    w1_ptr->set_modal(false);
    f.tick();
    assert(f.root->modal_top() == nullptr);
    assert(f.root->children().size() == base_children);
    std::printf("modal frame invariants ok\n");
}

// 6) 遮罩显式吞掉 click/right_click/double_click/long_press；默认不关闭窗口模态。
void test_scrim_swallows_gestures()
{
    Fixture f;
    auto outer = std::make_unique<GestureCounter>();
    GestureCounter* outer_ptr = outer.get();
    outer->set_pos(20.0f, 20.0f);
    f.add(std::move(outer));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{400.0f, 300.0f}, glm::vec2{200.0f, 150.0f});
    ui::Window* win_ptr = win.get();
    f.add(std::move(win));
    f.tick();

    win_ptr->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == win_ptr);

    // 点击遮罩（覆盖 outer 位置）：不穿透、不关闭模态。
    f.click({40.0f, 40.0f});
    assert(outer_ptr->clicks == 0);
    assert(f.root->modal_top() == win_ptr);

    // 右键：不穿透。
    f.click({40.0f, 40.0f}, /*button=*/1);
    assert(outer_ptr->right_clicks == 0);

    // 双击：不穿透。
    f.click({40.0f, 40.0f});
    f.click({40.0f, 40.0f});
    assert(outer_ptr->double_clicks == 0);

    // 长按：不穿透。
    f.system->set_long_press_ms(10.0f);
    f.down({40.0f, 40.0f});
    f.tick(0.02f); // 20ms > 10ms
    f.up({40.0f, 40.0f});
    assert(outer_ptr->long_presses == 0);

    // 模态仍打开（默认遮罩点击不关闭窗口）。
    assert(f.root->modal_top() == win_ptr);
    std::printf("scrim swallows gestures ok\n");
}

// 7) Scrim 的 dismiss 回调能力：设置后在遮罩按下时触发。
void test_scrim_dismiss_callback()
{
    Fixture f;
    auto scrim = std::make_unique<ui::Scrim>();
    int dismiss_calls = 0;
    scrim->set_on_dismiss([&dismiss_calls] { ++dismiss_calls; });
    f.add(std::move(scrim));
    f.tick();

    f.click({40.0f, 40.0f});
    assert(dismiss_calls == 1);
    std::printf("scrim dismiss callback ok\n");
}

// 8) 模态窗口被直接销毁时，模态栈清理悬空帧（modal_top 为空、焦点清空）。
void test_stale_modal_cleared()
{
    Fixture f;
    auto vbox = std::make_unique<ui::Vbox>(0.0f, 0.0f, 300.0f, 300.0f);
    ui::Vbox* vb = vbox.get();
    f.add(std::move(vbox));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{0.0f, 0.0f}, glm::vec2{150.0f, 100.0f});
    ui::Window* win_ptr = win.get();
    win->add(std::make_unique<ui::Button>("ok", [] {}));
    vb->add(std::move(win));
    f.tick();

    win_ptr->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == win_ptr);

    vb->clear_children(); // 绕过 release_modal 直接销毁
    f.tick();
    assert(f.root->modal_top() == nullptr);
    assert(f.focused() == nullptr);
    std::printf("stale modal cleared ok\n");
}
} // namespace

int main()
{
    test_key_char_blocked_outside_modal();
    test_initial_focus_moves_into_modal();
    test_focus_restored_on_close();
    test_nested_modal_focus_restore();
    test_dialog_modal_scope();
    test_dialog_over_modal_window();
    test_modal_frame_invariants();
    test_scrim_swallows_gestures();
    test_scrim_dismiss_callback();
    test_stale_modal_cleared();
    std::printf("ui modal tests passed\n");
    return 0;
}
