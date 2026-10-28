#include "ui/splitter.hpp"
#include "ui/label.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <utility>

namespace
{
using xel::ui::Label;
using xel::ui::Splitter;

constexpr float kEps = 1.0e-3f;

struct Fixture
{
    std::unique_ptr<Splitter> splitter;
    Label* first = nullptr;
    Label* second = nullptr;
};

Fixture make_splitter(Splitter::Orientation orientation = Splitter::Orientation::Horizontal)
{
    Fixture fixture;
    fixture.splitter = std::make_unique<Splitter>(0.0f, 0.0f, orientation);
    auto first = std::make_unique<Label>("first", 12.0f);
    auto second = std::make_unique<Label>("second", 12.0f);
    fixture.first = first.get();
    fixture.second = second.get();
    fixture.splitter->set_panes(std::move(first), std::move(second));
    return fixture;
}

// 比例模式：命中区跟着 ratio 走，拖动改 ratio。
void test_ratio_mode()
{
    Fixture f = make_splitter();
    f.splitter->set_ratio(0.30f);
    f.splitter->arrange({{0.0f, 0.0f}, {1000.0f, 200.0f}});

    // usable = 1000 - 6 = 994；first = 298.2；手柄在 298.2..304.2。
    assert(f.splitter->on_mouse_down({300.0f, 100.0f}, 0));
    assert(f.splitter->on_mouse_move({500.0f, 100.0f}));
    assert(f.splitter->on_mouse_up({500.0f, 100.0f}, 0));
    assert(std::fabs(f.splitter->ratio() - 500.0f / 994.0f) < kEps);
}

// 固定第一块：命中区在 280 附近（不是 ratio 默认的中间），拖动改第一块宽度。
void test_fixed_first()
{
    Fixture f = make_splitter();
    f.splitter->set_fixed_first(280.0f);
    f.splitter->arrange({{0.0f, 0.0f}, {1000.0f, 200.0f}});
    assert(std::fabs(f.first->width() - 280.0f) < kEps);
    assert(std::fabs(f.second->width() - (994.0f - 280.0f)) < kEps);

    // 中间（ratio 默认 0.5）不应命中——这正是「拖不动」的根因。
    assert(!f.splitter->on_mouse_down({500.0f, 100.0f}, 0));

    assert(f.splitter->on_mouse_down({283.0f, 100.0f}, 0));
    assert(f.splitter->on_mouse_move({400.0f, 100.0f}));
    assert(f.splitter->on_mouse_up({400.0f, 100.0f}, 0));
    f.splitter->arrange({{0.0f, 0.0f}, {1000.0f, 200.0f}});
    assert(std::fabs(f.first->width() - 400.0f) < kEps);
}

// 固定第二块（属性面板）：拖动改第二块宽度，第一块弹性。
void test_fixed_second()
{
    Fixture f = make_splitter();
    f.splitter->set_fixed_second(380.0f);
    f.splitter->arrange({{0.0f, 0.0f}, {1000.0f, 200.0f}});
    assert(std::fabs(f.second->width() - 380.0f) < kEps);

    // 手柄在 994 - 380 = 614 附近。
    assert(f.splitter->on_mouse_down({617.0f, 100.0f}, 0));
    assert(f.splitter->on_mouse_move({500.0f, 100.0f}));
    assert(f.splitter->on_mouse_up({500.0f, 100.0f}, 0));
    f.splitter->arrange({{0.0f, 0.0f}, {1000.0f, 200.0f}});
    assert(std::fabs(f.second->width() - (994.0f - 500.0f)) < kEps);
}

// 上下分割：固定第一块也走同一套逻辑。
void test_fixed_first_vertical()
{
    Fixture f = make_splitter(Splitter::Orientation::Vertical);
    f.splitter->set_fixed_first(120.0f);
    f.splitter->arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    assert(std::fabs(f.first->height() - 120.0f) < kEps);

    // usable = 600 - 6 = 594；手柄在 120..126。
    assert(f.splitter->on_mouse_down({200.0f, 123.0f}, 0));
    assert(f.splitter->on_mouse_move({200.0f, 300.0f}));
    assert(f.splitter->on_mouse_up({200.0f, 300.0f}, 0));
    f.splitter->arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    assert(std::fabs(f.first->height() - 300.0f) < kEps);
}
} // namespace

int main()
{
    test_ratio_mode();
    test_fixed_first();
    test_fixed_second();
    test_fixed_first_vertical();
    std::printf("test_ui_splitter: all passed\n");
    return 0;
}
