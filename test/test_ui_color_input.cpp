#include "ui/core/color_input.hpp"

#include "events/event.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
using xel::event::KeyCode;
using xel::event::Modifier;
using xel::ui::ColorInput;

constexpr float kEps = 1.0e-6f;

void test_clamp()
{
    ColorInput input("", 200.0f);
    input.set_value({2.0f, -1.0f, 0.5f, 3.0f});

    const glm::vec4 value = input.value();
    assert(std::fabs(value.r - 1.0f) < kEps);
    assert(std::fabs(value.g - 0.0f) < kEps);
    assert(std::fabs(value.b - 0.5f) < kEps);
    assert(std::fabs(value.a - 1.0f) < kEps);
}

void test_keyboard_cycles_palette()
{
    ColorInput input("", 200.0f);
    // 默认色板第 0 个：橙 (0.95, 0.55, 0.35)
    input.set_value({0.95f, 0.55f, 0.35f, 1.0f});

    int notifications = 0;
    glm::vec4 last{};
    input.set_on_change([&](glm::vec4 value) {
        ++notifications;
        last = value;
    });

    // 右移 → 第 1 个（黄，g ≈ 0.85）
    assert(input.on_key_down(KeyCode::Right, Modifier::None));
    assert(notifications == 1);
    assert(last.g > 0.8f);

    // 左移 → 回到橙
    assert(input.on_key_down(KeyCode::Left, Modifier::None));
    assert(notifications == 2);
    assert(last.g < 0.7f);

    // 从第 0 个左移应回绕到最后一个（白，r/g/b 都高）
    assert(input.on_key_down(KeyCode::Left, Modifier::None));
    assert(input.value().r > 0.85f);
    assert(input.value().b > 0.85f);
}

// 键盘切色上报 commit（before/after）。
void test_keyboard_commit()
{
    ColorInput input("", 200.0f);
    input.set_value({0.95f, 0.55f, 0.35f, 1.0f}); // 第 0 个

    int commits = 0;
    glm::vec4 last_before{};
    glm::vec4 last_after{};
    input.set_on_commit([&](glm::vec4 before, glm::vec4 after) {
        ++commits;
        last_before = before;
        last_after = after;
    });

    assert(input.on_key_down(KeyCode::Right, Modifier::None));
    assert(commits == 1);
    assert(last_before.g < 0.7f);  // 橙
    assert(last_after.g > 0.8f);   // 黄
}

void test_disabled_ignores_input()
{
    ColorInput input("", 200.0f);
    input.set_value({0.45f, 0.65f, 0.95f, 1.0f});
    input.set_enabled(false);

    assert(!input.on_key_down(KeyCode::Right, Modifier::None));
    assert(!input.on_mouse_down({10.0f, 15.0f}, 0));
    assert(std::fabs(input.value().g - 0.65f) < kEps);
}

void test_custom_palette()
{
    ColorInput input("", 200.0f);
    input.set_palette({{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
    input.set_value({0.0f, 0.0f, 0.0f, 1.0f});

    assert(input.on_key_down(KeyCode::Right, Modifier::None));
    assert(input.value().r > 0.99f);
    assert(input.on_key_down(KeyCode::Left, Modifier::None));
    assert(input.value().r < 0.01f);
}
} // namespace

int main()
{
    test_clamp();
    test_keyboard_cycles_palette();
    test_keyboard_commit();
    test_disabled_ignores_input();
    test_custom_palette();
    std::printf("test_ui_color_input: all passed\n");
    return 0;
}
