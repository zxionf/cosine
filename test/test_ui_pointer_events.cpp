#include "events/event.hpp"
#include "graphics/drawable.hpp"
#include "ui/root.hpp"
#include "ui/ui_system.hpp"
#include "ui/widget.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
using namespace xel;

// 固定尺寸的探针控件：记录收到的事件序列，不参与真实布局。
struct Probe : ui::Widget
{
    std::vector<std::string> events;
    glm::vec2 fixed_origin{0.0f, 0.0f};

    glm::vec2 measure(glm::vec2) override { return {200.0f, 60.0f}; }
    void arrange(ui::Rect) override { global_ = {fixed_origin, {200.0f, 60.0f}}; }
    void append_to(graphics::DrawList&) const override {}
    bool hit_testable() const override { return true; }
    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2, int) override { events.push_back("down"); return true; }
    bool on_mouse_up(glm::vec2, int) override { events.push_back("up"); return true; }
    bool on_click(glm::vec2, int) override { events.push_back("click"); return true; }
    bool on_right_click(glm::vec2) override { events.push_back("right"); return true; }
    bool on_double_click(glm::vec2, int) override { events.push_back("double"); return true; }
    bool on_long_press(glm::vec2, int) override { events.push_back("long"); return true; }

    int count(const std::string& what) const
    {
        int total = 0;
        for (const auto& name : events) {
            if (name == what) ++total;
        }
        return total;
    }
};

// 父容器：用于验证消费后不再冒泡。
struct Parent : ui::Widget
{
    std::vector<std::unique_ptr<ui::Widget>> kids;
    int double_seen = 0;

    const std::vector<std::unique_ptr<ui::Widget>>& children() const override { return kids; }
    glm::vec2 measure(glm::vec2) override { return {200.0f, 60.0f}; }
    void arrange(ui::Rect slot) override
    {
        global_ = slot;
        for (auto& kid : kids) kid->arrange(slot);
    }
    void append_to(graphics::DrawList&) const override {}
    bool on_double_click(glm::vec2, int) override
    {
        ++double_seen;
        return true;
    }
};

constexpr int kLeft = 0;
constexpr int kRight = 1;

struct Fixture
{
    Probe* probe = nullptr;
    std::unique_ptr<ui::UISystem> system;

    Fixture()
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto widget = std::make_unique<Probe>();
        probe = widget.get();
        root->add(std::move(widget));
        system = std::make_unique<ui::UISystem>(std::move(root));
        system->tick();
        probe->arrange({});
    }

    void mouse(event::InputEvent::Type type, glm::vec2 pos, int button)
    {
        event::InputEvent e;
        e.type = type;
        e.mouse_pos = pos;
        e.mouse_button = button;
        system->dispatch(e);
    }

    void click_at(glm::vec2 pos, int button = kLeft)
    {
        mouse(event::InputEvent::Type::MouseButtonDown, pos, button);
        mouse(event::InputEvent::Type::MouseButtonUp, pos, button);
    }
};

void test_left_click_focuses()
{
    Fixture f;
    f.click_at({50.0f, 30.0f});

    assert(f.probe->count("down") == 1);
    assert(f.probe->count("up") == 1);
    assert(f.probe->count("click") == 1);
    assert(f.probe->count("right") == 0);
    // 左键抢焦点。
    assert(f.system->focused() == f.probe);
}

void test_right_click_does_not_click_or_focus()
{
    Fixture f;
    f.click_at({50.0f, 30.0f}, kRight);

    assert(f.probe->count("right") == 1);
    assert(f.probe->count("click") == 0);
    // 右键不改变焦点。
    assert(f.system->focused() == nullptr);
}

void test_double_click_within_threshold()
{
    Fixture f;
    f.click_at({50.0f, 30.0f});
    f.system->tick(0.05f);
    f.click_at({52.0f, 31.0f});

    assert(f.probe->count("click") == 2);
    assert(f.probe->count("double") == 1);

    // 触发后清空：第三次点击不会连续再触发 double。
    f.click_at({52.0f, 31.0f});
    assert(f.probe->count("double") == 1);
}

void test_double_click_timeout()
{
    Fixture f;
    f.click_at({50.0f, 30.0f});
    f.system->tick(0.5f); // 500ms > 默认 400ms
    f.click_at({50.0f, 30.0f});

    assert(f.probe->count("click") == 2);
    assert(f.probe->count("double") == 0);
}

void test_double_click_slop()
{
    Fixture f;
    f.click_at({50.0f, 30.0f});
    f.system->tick(0.05f);
    f.click_at({80.0f, 30.0f}); // 偏移 30px > 默认 6px

    assert(f.probe->count("double") == 0);
}

void test_double_click_requires_same_target()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 500.0f, 300.0f);
    auto first = std::make_unique<Probe>();
    auto second = std::make_unique<Probe>();
    auto* a = first.get();
    auto* b = second.get();
    b->fixed_origin = {200.0f, 0.0f};
    root->add(std::move(first));
    root->add(std::move(second));

    ui::UISystem system{std::move(root)};
    system.tick();
    a->arrange({});
    b->arrange({});

    auto click = [&](glm::vec2 pos, int button) {
        event::InputEvent down;
        down.type = event::InputEvent::Type::MouseButtonDown;
        down.mouse_pos = pos;
        down.mouse_button = button;
        system.dispatch(down);
        event::InputEvent up = down;
        up.type = event::InputEvent::Type::MouseButtonUp;
        system.dispatch(up);
    };

    click({50.0f, 30.0f}, kLeft);
    system.tick(0.05f);
    click({250.0f, 30.0f}, kLeft); // 不同目标

    assert(a->count("double") == 0);
    assert(b->count("double") == 0);
}

void test_long_press_fires_once()
{
    Fixture f;
    f.mouse(event::InputEvent::Type::MouseButtonDown, {50.0f, 30.0f}, kLeft);

    f.system->tick(0.3f);
    assert(f.probe->count("long") == 0);

    f.system->tick(0.35f); // 累计 650ms > 默认 600ms
    assert(f.probe->count("long") == 1);

    f.system->tick(0.5f);
    assert(f.probe->count("long") == 1); // 只触发一次

    f.mouse(event::InputEvent::Type::MouseButtonUp, {50.0f, 30.0f}, kLeft);
    f.system->tick(0.5f);
    assert(f.probe->count("long") == 1);
}

void test_long_press_cancelled_when_pointer_leaves()
{
    Fixture f;
    f.mouse(event::InputEvent::Type::MouseButtonDown, {50.0f, 30.0f}, kLeft);
    // 指针移出探针范围（0..200 × 0..60）。
    f.mouse(event::InputEvent::Type::MouseMove, {300.0f, 200.0f}, kLeft);

    f.system->tick(0.7f);
    assert(f.probe->count("long") == 0);
}

void test_double_click_is_consumed()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    auto parent = std::make_unique<Parent>();
    auto child = std::make_unique<Probe>();
    auto* probe = child.get();
    auto* host = parent.get();
    child->set_parent(host);
    host->kids.push_back(std::move(child));
    root->add(std::move(parent));

    ui::UISystem system{std::move(root)};
    system.tick();
    probe->arrange({});

    auto click = [&](glm::vec2 pos) {
        event::InputEvent e;
        e.type = event::InputEvent::Type::MouseButtonDown;
        e.mouse_pos = pos;
        e.mouse_button = kLeft;
        system.dispatch(e);
        e.type = event::InputEvent::Type::MouseButtonUp;
        system.dispatch(e);
    };

    click({50.0f, 30.0f});
    system.tick(0.05f);
    click({50.0f, 30.0f});

    assert(probe->count("double") == 1);
    // 子控件消费后，父控件不再收到。
    assert(host->double_seen == 0);
}

void test_thresholds_configurable()
{
    Fixture f;
    f.system->set_double_click_ms(0.0f); // 关闭双击
    f.click_at({50.0f, 30.0f});
    f.system->tick(0.01f);
    f.click_at({50.0f, 30.0f});
    assert(f.probe->count("double") == 0);

    Fixture g;
    g.system->set_long_press_ms(0.0f); // 关闭长按
    g.mouse(event::InputEvent::Type::MouseButtonDown, {50.0f, 30.0f}, kLeft);
    g.system->tick(2.0f);
    assert(g.probe->count("long") == 0);
}

} // namespace

int main()
{
    test_left_click_focuses();
    test_right_click_does_not_click_or_focus();
    test_double_click_within_threshold();
    test_double_click_timeout();
    test_double_click_slop();
    test_double_click_requires_same_target();
    test_long_press_fires_once();
    test_long_press_cancelled_when_pointer_leaves();
    test_double_click_is_consumed();
    test_thresholds_configurable();
    std::cout << "ui pointer event tests passed\n";
    return 0;
}
