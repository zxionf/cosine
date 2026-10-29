#include "ui/composites/window.hpp"

#include "ui/containers/vbox.hpp"

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
    assert(std::fabs(f.window->absolute_pos().y - (800.0f - 32.0f)) < kEps);

    f.window->on_mouse_move({-5000.0f, -5000.0f});
    // min_x = 48 - 320 = -272；y 不小于 0（标题栏不能出顶部）
    assert(std::fabs(f.window->absolute_pos().x - (-272.0f)) < kEps);
    assert(std::fabs(f.window->absolute_pos().y - 0.0f) < kEps);
    f.window->on_mouse_up({0.0f, 0.0f}, 0);
}

// 点任意位置都置顶，且 z 会被归一化成 1..N。
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

    b->raise();
    assert(b->z_index() > a->z_index());

    // 点 A 的标题栏 → A 到最前。
    assert(a->on_mouse_down({20.0f, 10.0f}, 0));
    assert(a->z_index() > b->z_index());
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
    assert(std::fabs(f.window->height() - 32.0f) < kEps);
    f.window->set_collapsed(false);
    assert(std::fabs(f.window->height() - 200.0f) < kEps);

    // 关闭叉：右上角（用 global_bounds 算，父容器 padding 会让硬编码坐标偏）。
    bool destroyed = false;
    f.window->set_on_destroy([&destroyed] { destroyed = true; });
    const auto bounds = f.window->global_bounds();
    const glm::vec2 close_point{bounds.pos.x + bounds.size.x - 16.0f, bounds.pos.y + 16.0f};
    assert(f.window->on_mouse_down(close_point, 0));
    assert(destroyed);

    // 未注册销毁回调时退化为隐藏。
    Fixture g;
    g.window->destroy();
    assert(!g.window->visible());
}
} // namespace

int main()
{
    test_drag_follows_cursor();
    test_drag_clamped();
    test_raise_on_press();
    test_resize_grips();
    test_collapse_and_close();
    std::printf("test_ui_window: all passed\n");
    return 0;
}
