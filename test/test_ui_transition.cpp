#include "graphics/drawable.hpp"
#include "ui/button.hpp"
#include "ui/root.hpp"
#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
using namespace xel;

bool near(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

size_t vertex_count(const graphics::DrawList& list)
{
    size_t count = 0;
    for (const auto& batch : list.batches()) count += batch.vertices.size();
    return count;
}

// 每个用例独立构建一棵 UI 树：root + 一个 Button。
struct Fixture
{
    ui::Button* button = nullptr;
    std::unique_ptr<ui::UISystem> system;

    explicit Fixture(float transition_ms)
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto btn = std::make_unique<ui::Button>("ok");
        button = btn.get();
        root->add(std::move(btn));

        system = std::make_unique<ui::UISystem>(std::move(root));
        system->tick();
        system->set_theme(ui::Theme::dark().with_transition_ms(transition_ms));
        button->arrange({{0.0f, 0.0f}, {120.0f, 30.0f}});
    }

    const ui::Theme& theme() const { return system->context()->theme; }

    ui::BoxStyle style(ui::ControlState state) const
    {
        return button->visual_style(ui::StyleRole::Control, state);
    }
};

void test_zero_duration_is_immediate()
{
    Fixture f(0.0f);

    // 首次调用直接到位。
    assert(f.style(ui::ControlState::Normal).fill == f.theme().button_bg);

    // 状态切换也不会产生过渡：下一帧就是目标色。
    f.button->set_hovered(true);
    assert(f.style(ui::ControlState::Hovered).fill == f.theme().button_hover);
    assert(!f.button->transition_active());
}

void test_interpolates_to_target()
{
    Fixture f(100.0f);
    const auto& theme = f.theme();

    assert(f.style(ui::ControlState::Normal).fill == theme.button_bg);

    f.button->set_hovered(true);
    // 状态刚变化：从当前值（bg）出发，尚未推进。
    assert(f.style(ui::ControlState::Hovered).fill == theme.button_bg);

    // 推进一半时长：各分量严格介于两端之间。
    f.system->tick(0.05f);
    const auto mid = f.style(ui::ControlState::Hovered);
    assert(mid.fill.r > theme.button_bg.r);
    assert(mid.fill.r < theme.button_hover.r);
    assert(mid.fill.g > theme.button_bg.g);
    assert(mid.fill.g < theme.button_hover.g);
    assert(f.button->transition_active());

    // 补满时长后到位。
    f.system->tick(0.05f);
    const auto done = f.style(ui::ControlState::Hovered);
    assert(near(done.fill.r, theme.button_hover.r));
    assert(near(done.fill.a, theme.button_hover.a));
    assert(!f.button->transition_active());

    // 超出时长不会越界。
    f.system->tick(1.0f);
    assert(near(f.style(ui::ControlState::Hovered).fill.r, theme.button_hover.r));
}

void test_reverse_does_not_jump()
{
    Fixture f(100.0f);
    const auto& theme = f.theme();

    f.style(ui::ControlState::Normal);
    f.button->set_hovered(true);
    f.style(ui::ControlState::Hovered);

    f.system->tick(0.04f);
    const auto before = f.style(ui::ControlState::Hovered);
    assert(before.fill.r > theme.button_bg.r);

    // 中途撤销 hover：新过渡从「当前插值结果」出发，而不是跳回起点。
    f.button->set_hovered(false);
    const auto after = f.style(ui::ControlState::Normal);
    assert(near(after.fill.r, before.fill.r, 1e-2f));
}

void test_geometry_is_not_interpolated()
{
    Fixture f(100.0f);
    const ui::BoxStyle target = f.theme().style(ui::StyleRole::Control, ui::ControlState::Hovered);

    f.style(ui::ControlState::Normal);
    f.button->set_hovered(true);
    f.style(ui::ControlState::Hovered);
    f.system->tick(0.05f);

    const auto mid = f.style(ui::ControlState::Hovered);
    // 颜色在过渡，几何直接取目标值。
    assert(near(mid.radius, target.radius));
    assert(near(mid.border_width, target.border_width));
    assert(near(mid.text_size, target.text_size));
    assert(mid.fill.r < target.fill.r); // 仍在过渡中

    // 顶点数在过渡全程恒定（几何不变，只有顶点颜色变化）。
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    f.button->append_to(list);
    const size_t during = vertex_count(list);
    f.system->tick(0.05f);
    graphics::DrawList later;
    later.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    f.button->append_to(later);
    assert(vertex_count(later) == during);
}

void test_deterministic()
{
    auto sample = [] {
        Fixture f(100.0f);
        f.style(ui::ControlState::Normal);
        f.button->set_hovered(true);
        f.style(ui::ControlState::Hovered);
        f.system->tick(0.03f);
        f.system->tick(0.02f);
        return f.style(ui::ControlState::Hovered).fill.r;
    };

    const float first = sample();
    const float second = sample();
    assert(near(first, second, 1e-6f));
}

void test_disabled_transition()
{
    Fixture f(100.0f);
    const auto& theme = f.theme();

    f.style(ui::ControlState::Normal);
    f.button->set_enabled(false);
    assert(f.style(ui::ControlState::Disabled).fill == theme.button_bg);

    f.system->tick(0.1f);
    assert(near(f.style(ui::ControlState::Disabled).fill.r, theme.button_disabled.r));
    // 文字颜色也插值到禁用色。
    assert(near(f.style(ui::ControlState::Disabled).text.a, theme.text_disabled.a));
}

void test_tick_without_dt_keeps_working()
{
    // 无参 tick() 等价于 1/60s，且不改变已完成的状态。
    Fixture f(120.0f);
    f.style(ui::ControlState::Normal);
    f.system->tick();
    assert(f.style(ui::ControlState::Normal).fill == f.theme().button_bg);
}

void test_theme_change_applies_immediately()
{
    Fixture f(100.0f);
    const auto before = f.style(ui::ControlState::Normal);
    assert(before.fill == f.theme().button_bg);

    // 切主题后下一次取样式立即反映新主题，不用等 hover 触发状态变化。
    f.system->set_theme(ui::Theme::light().with_transition_ms(100.0f));
    const auto after = f.style(ui::ControlState::Normal);
    assert(after.fill == f.theme().button_bg);
    assert(after.fill != before.fill);
    assert(!f.button->transition_active());

    // 圆角开关同样立即生效。
    f.system->set_theme(ui::Theme::light().with_rounding(false).with_transition_ms(100.0f));
    assert(f.style(ui::ControlState::Normal).radius == 0.0f);

    // 全局 set_theme 也走同一条失效路径。
    ui::set_theme(ui::Theme::dark().with_transition_ms(100.0f));
    assert(f.style(ui::ControlState::Normal).fill == f.theme().button_bg);
}

} // namespace

int main()
{
    test_zero_duration_is_immediate();
    test_interpolates_to_target();
    test_reverse_does_not_jump();
    test_geometry_is_not_interpolated();
    test_deterministic();
    test_disabled_transition();
    test_tick_without_dt_keeps_working();
    test_theme_change_applies_immediately();
    std::cout << "ui transition tests passed\n";
    return 0;
}
