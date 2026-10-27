#include "ui/drag_float.hpp"

#include "events/event.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
using xel::event::KeyCode;
using xel::event::Modifier;
using xel::ui::DragFloat;

constexpr float kEps = 1.0e-4f;

void test_clamp()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 10.0f);

    drag.set_value(20.0f);
    assert(std::fabs(drag.value() - 10.0f) < kEps);

    drag.set_value(-5.0f);
    assert(std::fabs(drag.value() - 0.0f) < kEps);

    // 反向 range 也能正确归一（set_range 内部取 min/max）。
    drag.set_range(10.0f, 0.0f);
    drag.set_value(5.0f);
    assert(std::fabs(drag.value() - 5.0f) < kEps);
}

void test_drag_changes_value()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 100.0f);
    drag.set_value(50.0f);
    drag.arrange({{0.0f, 0.0f}, {100.0f, 30.0f}});

    int notifications = 0;
    float last = 0.0f;
    drag.set_on_change([&](float value) {
        ++notifications;
        last = value;
    });

    assert(drag.on_mouse_down({10.0f, 10.0f}, 0));
    // 默认灵敏度：200 像素走完全程 → 100/200 = 0.5/px；右移 50px → +25。
    assert(drag.on_mouse_move({60.0f, 10.0f}));
    assert(std::fabs(last - 75.0f) < kEps);
    assert(notifications == 1);

    // 松开后不再响应移动。
    assert(drag.on_mouse_up({60.0f, 10.0f}, 0));
    assert(!drag.on_mouse_move({90.0f, 10.0f}));
    assert(notifications == 1);
}

void test_explicit_step()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 1000.0f);
    drag.set_value(0.0f);
    drag.set_step(2.0f);

    float last = 0.0f;
    drag.set_on_change([&](float value) { last = value; });

    assert(drag.on_mouse_down({0.0f, 0.0f}, 0));
    assert(drag.on_mouse_move({10.0f, 0.0f}));
    assert(std::fabs(last - 20.0f) < kEps);
}

void test_keyboard()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 100.0f);
    drag.set_value(50.0f);
    drag.set_step(2.0f);

    assert(drag.on_key_down(KeyCode::Right, Modifier::None));
    assert(std::fabs(drag.value() - 52.0f) < kEps);
    assert(drag.on_key_down(KeyCode::Left, Modifier::None));
    assert(std::fabs(drag.value() - 50.0f) < kEps);

    assert(drag.on_key_down(KeyCode::Home, Modifier::None));
    assert(std::fabs(drag.value() - 0.0f) < kEps);
    assert(drag.on_key_down(KeyCode::End, Modifier::None));
    assert(std::fabs(drag.value() - 100.0f) < kEps);

    // 未处理的键返回 false。
    assert(!drag.on_key_down(KeyCode::Tab, Modifier::None));
}

void test_disabled_ignores_input()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 10.0f);
    drag.set_value(5.0f);
    drag.set_enabled(false);

    assert(!drag.on_mouse_down({0.0f, 0.0f}, 0));
    assert(!drag.on_key_down(KeyCode::Right, Modifier::None));
    assert(std::fabs(drag.value() - 5.0f) < kEps);
}

void test_focus_lost_stops_drag()
{
    DragFloat drag("X", 100.0f);
    drag.set_range(0.0f, 100.0f);
    drag.set_value(50.0f);

    assert(drag.on_mouse_down({0.0f, 0.0f}, 0));
    drag.on_focus_lost();
    assert(!drag.on_mouse_move({100.0f, 0.0f}));
}
} // namespace

int main()
{
    test_clamp();
    test_drag_changes_value();
    test_explicit_step();
    test_keyboard();
    test_disabled_ignores_input();
    test_focus_lost_stops_drag();
    std::printf("test_ui_drag_float: all passed\n");
    return 0;
}
