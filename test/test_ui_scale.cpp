// 缩放/坐标换算测试：framebuffer→logical、pointer_pos 更新、ui_scale 设置。
#include "events/event.hpp"
#include "ui/system/root.hpp"
#include "ui/system/ui_system.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

using namespace xel;

namespace
{
void test_framebuffer_to_logical()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
    ui::UISystem ui{std::move(root)};

    ui.set_ui_scale(2.0f);
    glm::vec2 logical = ui.framebuffer_to_logical({400.0f, 300.0f});
    assert(logical.x == 200.0f);
    assert(logical.y == 150.0f);

    ui.set_ui_scale(1.0f);
    logical = ui.framebuffer_to_logical({400.0f, 300.0f});
    assert(logical.x == 400.0f);
    assert(logical.y == 300.0f);
    std::printf("framebuffer to logical ok\n");
}

void test_pointer_pos_updated_on_dispatch()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
    ui::UISystem ui{std::move(root)};

    // 初始为 (0,0)。
    assert(ui.pointer_pos().x == 0.0f && ui.pointer_pos().y == 0.0f);

    event::InputEvent move;
    move.type = event::InputEvent::Type::MouseMove;
    move.mouse_pos = {123.0f, 456.0f};
    ui.dispatch(move);

    assert(ui.pointer_pos().x == 123.0f);
    assert(ui.pointer_pos().y == 456.0f);
    std::printf("pointer pos updated ok\n");
}

void test_ui_scale_setter()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
    ui::UISystem ui{std::move(root)};

    ui.set_ui_scale(3.0f);
    assert(ui.ui_scale() == 3.0f);

    // 非法值（<= 0）回退到 1.0，保持旧行为。
    ui.set_ui_scale(0.0f);
    assert(ui.ui_scale() == 1.0f);
    std::printf("ui scale setter ok\n");
}
} // namespace

int main()
{
    test_framebuffer_to_logical();
    test_pointer_pos_updated_on_dispatch();
    test_ui_scale_setter();
    std::printf("ui scale tests passed\n");
    return 0;
}
