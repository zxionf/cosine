#include "ui/root.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
using xel::ui::Rect;
using xel::ui::Widget;

class TestWidget : public Widget
{
public:
    TestWidget(float x, float y, float w, float h)
    {
        set_pos(x, y);
        set_size(w, h);
    }

    glm::vec2 measure(glm::vec2) override { return local_bounds().size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(xel::graphics::DrawList&) const override
    {
        if (draw_log) draw_log->push_back(draw_id);
    }

    bool hit_testable() const override { return hit_testable_; }
    void set_hit_testable(bool value) { hit_testable_ = value; }
    bool focusable() const override { return focusable_; }
    void set_focusable(bool value) { focusable_ = value; }

    bool on_mouse_down(glm::vec2, int) override { ++down; return consume_down; }
    bool on_mouse_up(glm::vec2, int) override { ++up; return consume_up; }
    bool on_click(glm::vec2, int) override { ++click; return false; }
    bool on_mouse_move(glm::vec2) override { ++move; return false; }
    bool on_key_down(int, int) override { ++key_down; return false; }
    bool on_key_up(int, int) override { ++key_up; return false; }
    bool on_char(char32_t) override { ++chars; return false; }
    void on_focus_gained() override { ++focus_gained; }
    void on_focus_lost() override { ++focus_lost; }

    bool consume_down = false;
    bool consume_up = false;

    int down = 0;
    int up = 0;
    int click = 0;
    int move = 0;
    int key_down = 0;
    int key_up = 0;
    int chars = 0;
    int focus_gained = 0;
    int focus_lost = 0;

    static inline std::vector<int>* draw_log = nullptr;
    int draw_id = 0;

private:
    bool focusable_ = false;
    bool hit_testable_ = true;
};

class TestContainer : public Widget
{
public:
    TestContainer(float x, float y, float w, float h)
    {
        set_pos(x, y);
        set_size(w, h);
    }

    void add(std::unique_ptr<Widget> child) override
    {
        child->set_parent(this);
        children_.push_back(std::move(child));
        mark_layout_dirty();
    }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

    glm::vec2 measure(glm::vec2) override { return local_bounds().size; }
    void arrange(Rect slot) override
    {
        global_ = slot;
        for (auto& child : children_) {
            child->arrange({{slot.pos.x + child->x(), slot.pos.y + child->y()}, child->measure(slot.size)});
        }
    }
    void append_to(xel::graphics::DrawList&) const override {}

    bool on_mouse_down(glm::vec2, int) override { ++down; return consume_down; }
    bool on_mouse_up(glm::vec2, int) override { ++up; return false; }

    bool consume_down = false;
    int down = 0;
    int up = 0;

private:
    std::vector<std::unique_ptr<Widget>> children_;
};

struct Fixture
{
    struct Tree
    {
        std::unique_ptr<xel::ui::UIRoot> root;
        TestContainer* container = nullptr;
        TestWidget* a = nullptr;
        TestWidget* b = nullptr;
    };

    static Tree make_tree()
    {
        Tree tree;
        tree.root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);

        auto container = std::make_unique<TestContainer>(0.0f, 0.0f, 800.0f, 600.0f);
        tree.container = container.get();

        auto a = std::make_unique<TestWidget>(0.0f, 0.0f, 100.0f, 100.0f);
        tree.a = a.get();
        auto b = std::make_unique<TestWidget>(0.0f, 0.0f, 100.0f, 100.0f);
        tree.b = b.get();

        container->add(std::move(a));
        container->add(std::move(b));
        tree.root->add(std::move(container));

        tree.container->set_global_bounds({{0.0f, 0.0f}, {800.0f, 600.0f}});
        tree.a->set_global_bounds({{0.0f, 0.0f}, {100.0f, 100.0f}});
        tree.b->set_global_bounds({{0.0f, 0.0f}, {100.0f, 100.0f}});

        return tree;
    }

    Tree tree;
    xel::ui::UISystem ui;

    Fixture() : tree{make_tree()}, ui{std::move(tree.root)} {}

    xel::event::InputEvent mouse(xel::event::InputEvent::Type type, glm::vec2 pos, int button = 0)
    {
        xel::event::InputEvent e;
        e.type = type;
        e.mouse_pos = pos;
        e.mouse_button = button;
        return e;
    }
};
} // namespace

int main()
{
    {
        Fixture f;
        f.tree.b->consume_down = true;

        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonDown, {50.0f, 50.0f}));
        assert(f.tree.b->down == 1);
        assert(f.tree.a->down == 0);
        assert(f.tree.container->down == 0);

        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonUp, {50.0f, 50.0f}));
        assert(f.tree.b->up == 1);
        assert(f.tree.b->click == 1);
    }

    {
        Fixture f;

        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonDown, {50.0f, 50.0f}));
        assert(f.tree.b->down == 1);
        assert(f.tree.container->down == 1);
        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonUp, {50.0f, 50.0f}));
    }

    {
        Fixture f;
        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonDown, {50.0f, 50.0f}));
        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseMove, {500.0f, 500.0f}));
        assert(f.tree.b->move == 1);
        assert(f.tree.a->move == 0);

        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonUp, {500.0f, 500.0f}));
        assert(f.tree.b->up == 1);
        assert(f.tree.b->click == 0);
    }

    {
        Fixture f;
        f.tree.b->set_focusable(true);
        f.tree.a->set_focusable(true);

        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonDown, {50.0f, 50.0f}));
        f.ui.dispatch(f.mouse(xel::event::InputEvent::Type::MouseButtonUp, {50.0f, 50.0f}));
        assert(f.ui.focused() == f.tree.b);
        assert(f.tree.b->focus_gained == 1);

        f.ui.focus_next(false);
        assert(f.ui.focused() == f.tree.a);
        assert(f.tree.b->focus_lost == 1);
        assert(f.tree.a->focus_gained == 1);

        xel::event::InputEvent key;
        key.type = xel::event::InputEvent::Type::KeyDown;
        key.key = 65;
        f.ui.dispatch(key);
        assert(f.tree.a->key_down == 1);

        xel::event::InputEvent tab;
        tab.type = xel::event::InputEvent::Type::KeyDown;
        tab.key = GLFW_KEY_TAB;
        f.ui.dispatch(tab);
        assert(f.ui.focused() == f.tree.b);

        xel::event::InputEvent ch;
        ch.type = xel::event::InputEvent::Type::CharInput;
        ch.codepoint = U'x';
        f.ui.dispatch(ch);
        assert(f.tree.b->chars == 1);
    }

    {
        std::vector<int> order;
        TestWidget::draw_log = &order;

        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 200.0f, 200.0f);
        auto normal = std::make_unique<TestWidget>(0.0f, 0.0f, 200.0f, 200.0f);
        normal->draw_id = 1;
        auto overlay = std::make_unique<TestWidget>(20.0f, 20.0f, 100.0f, 100.0f);
        overlay->draw_id = 2;
        auto* normal_ptr = normal.get();
        auto* overlay_ptr = overlay.get();
        root->add(std::move(normal));
        root->add_overlay(std::move(overlay));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        xel::graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 200.0f, 200.0f});
        ui.root()->append_to(list);
        assert(order.size() == 2);
        assert(order[0] == 1 && order[1] == 2);

        auto mouse = [](xel::event::InputEvent::Type type, glm::vec2 pos) {
            xel::event::InputEvent e;
            e.type = type;
            e.mouse_pos = pos;
            return e;
        };

        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {50.0f, 50.0f}));
        assert(overlay_ptr->down == 1);
        assert(normal_ptr->down == 0);
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, {50.0f, 50.0f}));
        assert(overlay_ptr->up == 1);

        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {150.0f, 150.0f}));
        assert(normal_ptr->down == 1);

        TestWidget::draw_log = nullptr;
    }

    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 200.0f, 200.0f);
        auto normal = std::make_unique<TestWidget>(0.0f, 0.0f, 200.0f, 200.0f);
        auto pass = std::make_unique<TestWidget>(0.0f, 0.0f, 200.0f, 200.0f);
        pass->set_hit_testable(false);
        auto* normal_ptr = normal.get();
        auto* pass_ptr = pass.get();
        root->add(std::move(normal));
        root->add_overlay(std::move(pass));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        xel::event::InputEvent down;
        down.type = xel::event::InputEvent::Type::MouseButtonDown;
        down.mouse_pos = {50.0f, 50.0f};
        ui.dispatch(down);
        assert(normal_ptr->down == 1);
        assert(pass_ptr->down == 0);
    }

    std::cout << "ui event tests passed\n";
    return 0;
}
