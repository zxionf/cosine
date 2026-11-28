// UiRuntime 契约测试：UI 运行时不依赖平台/渲染后端。
//
// 这个测试**只链接 cosine_ui**（不含 backend/vulkan），本身就是约束的证明：
// 如果 UiRuntime 哪天又开始 include 平台头，这里会连编译都过不去。
#include "ui/core/button.hpp"
#include "ui/input/cursor.hpp"
#include "ui/system/ui_runtime.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace xel;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const std::string& name, const std::string& detail = std::string())
{
    if (ok) {
        ++g_passed;
        std::printf("ok      %s\n", name.c_str());
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name.c_str(), detail.c_str());
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// 记录宿主回调：光标与渲染缩放都必须经过这里，而不是 UiRuntime 自己乱调。
struct HostProbe
{
    std::vector<ui::CursorShape> cursors;
    std::vector<float> scales;

    ui::UiRuntime::Hooks hooks()
    {
        return {
            [this](ui::CursorShape shape) { cursors.push_back(shape); },
            [this](float scale) { scales.push_back(scale); },
        };
    }
};
} // namespace

// 1) 视图尺寸：物理像素 ÷ ui_scale = root 逻辑尺寸，且只在 tick 里落地。
void test_viewport_and_scale()
{
    ui::UiRuntime runtime;
    HostProbe host;
    runtime.bind_host(host.hooks());

    check(near(runtime.root().width(), 0.0f) && near(runtime.root().height(), 0.0f),
          "root starts empty");

    runtime.set_viewport({800.0f, 600.0f});
    check(near(runtime.root().width(), 0.0f), "viewport is deferred (not applied on the spot)");

    runtime.tick();
    check(near(runtime.root().width(), 800.0f) && near(runtime.root().height(), 600.0f),
          "tick applies viewport to root");

    runtime.set_ui_scale(2.0f);
    check(host.scales.size() == 2 && near(host.scales.back(), 2.0f),
          "set_ui_scale fans out to the render-scale hook once",
          "calls=" + std::to_string(host.scales.size()));
    runtime.tick();
    check(near(runtime.root().width(), 400.0f) && near(runtime.root().height(), 300.0f),
          "root uses logical pixels (framebuffer / ui_scale)");

    check(near(runtime.framebuffer_to_logical({800.0f, 600.0f}).x, 400.0f),
          "framebuffer_to_logical divides by ui_scale");
}

// 2) 同一个 DPI 入口不会重复扇出，且宿主晚绑定也会补一次。
void test_scale_single_source()
{
    ui::UiRuntime runtime;
    runtime.set_ui_scale(1.5f); // 宿主尚未绑定：不崩，也不丢状态
    check(near(runtime.ui_scale(), 1.5f), "scale is stored before a host is bound");

    HostProbe host;
    runtime.bind_host(host.hooks());
    check(host.scales.size() == 1 && near(host.scales.back(), 1.5f),
          "late bind pushes the current scale once");

    runtime.set_ui_scale(1.5f);
    check(host.scales.size() == 1, "same scale is not fanned out twice");

    runtime.set_ui_scale(100.0f); // 超界：夹到 4×
    check(near(runtime.ui_scale(), 4.0f), "ui scale is clamped to [0.25, 4]");
    runtime.set_ui_scale(0.0f);   // 非正数：夹到 0.25×
    check(near(runtime.ui_scale(), 0.25f), "non-positive scale falls back to the minimum");
}

// 3) 树与事件：UiRuntime 是唯一入口，不需要窗口就能命中与派发。
void test_tree_and_dispatch()
{
    ui::UiRuntime runtime;
    runtime.set_viewport({400.0f, 300.0f});
    runtime.tick();

    auto button = std::make_unique<ui::Button>("OK");
    button->set_pos(10.0f, 10.0f);
    button->set_size(80.0f, 30.0f);
    int clicks = 0;
    button->set_on_click([&clicks] { ++clicks; });
    runtime.root().add(std::move(button));

    event::InputEvent move;
    move.type = event::InputEvent::Type::MouseMove;
    move.mouse_pos = {20.0f, 20.0f};
    runtime.dispatch(move);
    check(runtime.hovered() != nullptr, "dispatch drives hover without a window");

    event::InputEvent down;
    down.type = event::InputEvent::Type::MouseButtonDown;
    down.mouse_button = 0;
    down.mouse_pos = {20.0f, 20.0f};
    runtime.dispatch(down);
    check(runtime.focused() != nullptr, "left press focuses the control");

    event::InputEvent up = down;
    up.type = event::InputEvent::Type::MouseButtonUp;
    runtime.dispatch(up);
    check(clicks == 1, "click still reaches the control", "clicks=" + std::to_string(clicks));

    check(near(runtime.pointer_pos().x, 20.0f) && near(runtime.pointer_pos().y, 20.0f),
          "pointer_pos caches the logical position");
}

// 4) 光标回调与字体图集注入（不引入任何 graphics 依赖即可验证回调打通）。
void test_host_hooks()
{
    ui::UiRuntime runtime;
    HostProbe host;
    runtime.bind_host(host.hooks());
    runtime.set_viewport({200.0f, 100.0f});
    runtime.tick();

    // 默认没有控件自报光标：一次移动只应把 Arrow 推给宿主。
    event::InputEvent move;
    move.type = event::InputEvent::Type::MouseMove;
    move.mouse_pos = {5.0f, 5.0f};
    runtime.dispatch(move);
    check(!host.cursors.empty() && host.cursors.back() == ui::CursorShape::Arrow,
          "idle hover reports the arrow cursor");

    runtime.set_font_atlas(nullptr);
    check(runtime.context() != nullptr && runtime.context()->font_atlas == nullptr,
          "font atlas is injectable through the runtime");
}

int main()
{
    test_viewport_and_scale();
    test_scale_single_source();
    test_tree_and_dispatch();
    test_host_hooks();

    std::printf("ui runtime: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
