#include "ui_demo.hpp"
#include "xel/version.hpp"
#include "xel_application.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <string_view>

namespace
{
// 边沿检测：按住不放只触发一次。
struct KeyLatch
{
    xel::event::KeyCode key = xel::event::KeyCode::Unknown;
    bool prev = false;
    bool pressed(bool down)
    {
        const bool fired = down && !prev;
        prev = down;
        return fired;
    }
};

int run()
{
    xel::Application app{{1200, 800, "XEL Text Lab"}};
    auto lab = xel::demo::build(app.ui().root());

    KeyLatch f1{xel::event::KeyCode::F1};
    KeyLatch f2{xel::event::KeyCode::F2};
    KeyLatch f3{xel::event::KeyCode::F3};
    KeyLatch f4{xel::event::KeyCode::F4};
    KeyLatch f5{xel::event::KeyCode::F5};

    // 每帧：F1–F5 调试开关 + 覆盖层状态跟着指针/焦点刷新。
    return app.run([&](float) {
        if (f1.pressed(app.window().key_down(f1.key))) lab.overlay->toggle();
        if (f2.pressed(app.window().key_down(f2.key))) lab.toggle_glyph_boxes();
        if (f3.pressed(app.window().key_down(f3.key))) lab.toggle_line_box();
        if (f4.pressed(app.window().key_down(f4.key))) lab.toggle_advance();
        if (f5.pressed(app.window().key_down(f5.key))) lab.toggle_pixel_snap();

        if (lab.overlay->visible()) {
            lab.overlay->update(app.gfx().stats(), app.ui().hovered(),
                                app.ui().focused(), {0.0f, 0.0f});
        }
    });
}
} // namespace

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--version") {
            std::cout << xel::kProjectName << " " << xel::version_string() << '\n';
            return 0;
        }
    }

    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
