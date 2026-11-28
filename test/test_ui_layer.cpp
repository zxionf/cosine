// Layer 排序契约测试：layer 是子项的主排序键，z 仅在层内生效。
// 覆盖：layer_paint_order 顺序、命中优先级、模态盖浮层、嵌套模态 z、悬空帧清理。
#include "events/event.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/composites/window.hpp"
#include "ui/system/root.hpp"
#include "ui/system/ui_system.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

using namespace xel;

namespace
{
// 测试用控件：显式 layer 与 hit_testable，固定几何 100x100，点击计数。
struct Layered : ui::Widget
{
    ui::Widget::Layer layer_;
    bool hit_;
    int clicks = 0;

    Layered(ui::Widget::Layer layer, bool hit = true) : layer_(layer), hit_(hit)
    {
        set_size({100.0f, 100.0f});
    }

    ui::Widget::Layer layer() const override { return layer_; }
    bool hit_testable() const override { return hit_; }
    glm::vec2 measure(glm::vec2) override { return {100.0f, 100.0f}; }
    void arrange(ui::Rect slot) override
    {
        set_geometry(slot);
    }
    void append_to(graphics::DrawList&) const override {}
    bool on_click(glm::vec2, int) override
    {
        ++clicks;
        return true;
    }
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
    void tick() { system->tick(); }

    void click(glm::vec2 pos)
    {
        event::InputEvent down;
        down.type = event::InputEvent::Type::MouseButtonDown;
        down.mouse_pos = pos;
        down.mouse_button = 0;
        system->dispatch(down);

        event::InputEvent up;
        up.type = event::InputEvent::Type::MouseButtonUp;
        up.mouse_pos = pos;
        up.mouse_button = 0;
        system->dispatch(up);
    }
};

void test_layer_paint_order_basic()
{
    Fixture f;
    auto modal = std::make_unique<Layered>(ui::Widget::Layer::Modal);
    auto floating = std::make_unique<Layered>(ui::Widget::Layer::Floating);
    auto content = std::make_unique<Layered>(ui::Widget::Layer::Content);
    Layered* m = modal.get();
    Layered* fl = floating.get();
    Layered* c = content.get();
    f.add(std::move(modal));
    f.add(std::move(floating));
    f.add(std::move(content));

    const auto order = f.root->layer_paint_order();
    assert(order.size() == 3);
    assert(order[0] == c); // Content 最低
    assert(order[1] == fl);
    assert(order[2] == m); // Modal 最高
    std::printf("layer paint order basic ok\n");
}

void test_layer_z_within_layer()
{
    Fixture f;
    auto low = std::make_unique<Layered>(ui::Widget::Layer::Content);
    auto high = std::make_unique<Layered>(ui::Widget::Layer::Content);
    Layered* lo = low.get();
    Layered* hi = high.get();
    hi->set_z_index(5);
    lo->set_z_index(0);
    f.add(std::move(low));
    f.add(std::move(high));

    const auto order = f.root->layer_paint_order();
    assert(order.size() == 2);
    assert(order[0] == lo); // 同层按 z 升序
    assert(order[1] == hi);
    std::printf("layer z within layer ok\n");
}

void test_hit_floating_over_content()
{
    Fixture f;
    auto content = std::make_unique<Layered>(ui::Widget::Layer::Content);
    auto floating = std::make_unique<Layered>(ui::Widget::Layer::Floating);
    Layered* c = content.get();
    Layered* fl = floating.get();
    content->set_pos(0.0f, 0.0f);
    floating->set_pos(0.0f, 0.0f); // 完全重叠
    f.add(std::move(content));
    f.add(std::move(floating));
    f.tick();

    f.click({50.0f, 50.0f});
    assert(fl->clicks == 1); // 浮层先命中
    assert(c->clicks == 0);
    std::printf("hit floating over content ok\n");
}

void test_hit_modal_over_floating()
{
    Fixture f;
    auto floating = std::make_unique<Layered>(ui::Widget::Layer::Floating);
    auto modal = std::make_unique<Layered>(ui::Widget::Layer::Modal);
    Layered* fl = floating.get();
    Layered* m = modal.get();
    floating->set_pos(0.0f, 0.0f);
    modal->set_pos(0.0f, 0.0f); // 完全重叠
    f.add(std::move(floating));
    f.add(std::move(modal));
    f.tick();

    f.click({50.0f, 50.0f});
    assert(m->clicks == 1); // Modal 层先命中
    assert(fl->clicks == 0);
    std::printf("hit modal over floating ok\n");
}

void test_nested_modal_z_normalized()
{
    Fixture f;
    auto w1 = std::make_unique<ui::Window>("A", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 100.0f});
    auto w2 = std::make_unique<ui::Window>("B", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 100.0f});
    ui::Window* p1 = w1.get();
    ui::Window* p2 = w2.get();
    f.add(std::move(w1));
    f.add(std::move(w2));
    f.tick();

    p1->set_modal(true);
    p2->set_modal(true);
    f.tick(); // layout 归一化 z

    assert(p1->z_index() == 2); // frame 0: scrim 1 / window 2
    assert(p2->z_index() == 4); // frame 1: scrim 3 / window 4
    assert(f.root->modal_top() == p2);
    std::printf("nested modal z normalized ok\n");
}

void test_stale_modal_frame_pruned()
{
    Fixture f;
    auto vbox = std::make_unique<ui::Vbox>(0.0f, 0.0f, 200.0f, 200.0f);
    ui::Vbox* vb = vbox.get();
    f.add(std::move(vbox));

    auto win = std::make_unique<ui::Window>("W", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 100.0f});
    ui::Window* w = win.get();
    vb->add(std::move(win));
    f.tick();

    w->set_modal(true);
    f.tick();
    assert(f.root->modal_top() == w);

    // 直接销毁窗口（绕过 release_modal）：layout 应剔除悬空帧并移除残留 scrim。
    vb->clear_children();
    f.tick();
    assert(f.root->modal_top() == nullptr);
    std::printf("stale modal frame pruned ok\n");
}
} // namespace

int main()
{
    test_layer_paint_order_basic();
    test_layer_z_within_layer();
    test_hit_floating_over_content();
    test_hit_modal_over_floating();
    test_nested_modal_z_normalized();
    test_stale_modal_frame_pruned();
    std::printf("ui layer tests passed\n");
    return 0;
}
