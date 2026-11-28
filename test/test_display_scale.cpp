// DisplayScale 契约测试：dpr 与 ui_scale 的换算口径只有一处定义。
//
// 这两个数字之前散在 Window / UiRuntime / Renderer / Application 四处各算一遍，
// 这里把「谁乘谁、谁除谁」钉死，避免以后又出现「字模糊 / 命中偏移」。
#include "graphics/display_scale.hpp"

#include <cmath>
#include <cstdio>

using xel::graphics::DisplayScale;
using glm::vec2;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* name, const char* detail = "")
{
    if (ok) {
        std::printf("ok      %s\n", name);
        ++g_passed;
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name, detail);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
bool near(vec2 a, vec2 b) { return near(a.x, b.x) && near(a.y, b.y); }
} // namespace

// 1) 默认值与非法输入：垃圾值一律回退到安全默认，而不是把界面缩到最小。
void test_defaults_and_clamp()
{
    const DisplayScale identity{};
    check(near(identity.dpr, 1.0f) && near(identity.ui_scale, 1.0f), "defaults are 1:1");

    check(near(DisplayScale::make(0.0f, 2.0f).dpr, 1.0f), "non-positive dpr falls back to 1.0");
    check(near(DisplayScale::make(2.0f, 0.0f).ui_scale, 1.0f),
          "non-positive ui_scale falls back to 1.0");
    check(near(DisplayScale::make(2.0f, 100.0f).ui_scale, DisplayScale::kMaxUiScale),
          "too-large ui_scale clamps to the maximum");
    check(near(DisplayScale::make(2.0f, 0.1f).ui_scale, DisplayScale::kMinUiScale),
          "too-small ui_scale clamps to the minimum");
    check(near(DisplayScale::make(std::nanf(""), 2.0f).dpr, 1.0f), "NaN dpr falls back to 1.0");

    const DisplayScale scaled = DisplayScale::make(2.0f, 2.0f);
    check(near(scaled.with_dpr(3.0f).dpr, 3.0f) && near(scaled.with_dpr(3.0f).ui_scale, 2.0f),
          "with_dpr changes only dpr");
    check(near(scaled.with_ui_scale(0.5f).ui_scale, 0.5f) && near(scaled.with_ui_scale(0.5f).dpr, 2.0f),
          "with_ui_scale changes only ui_scale");
}

// 2) 两条换算路径：物理 → 逻辑（尺寸），窗口 → 逻辑（输入）。
void test_conversions()
{
    // 2× 显示器 + 2× 用户比例：1 物理像素 = 0.5 逻辑像素；1 窗口坐标 = 1 逻辑像素。
    const DisplayScale scale = DisplayScale::make(2.0f, 2.0f);
    check(near(scale.framebuffer_to_logical({800.0f, 600.0f}), {400.0f, 300.0f}),
          "framebuffer size divides by ui_scale only");
    check(near(scale.window_to_logical({400.0f, 300.0f}), {400.0f, 300.0f}),
          "window coords multiply by dpr/ui_scale");

    const DisplayScale hidpi_only = DisplayScale::make(2.0f, 1.0f);
    check(near(hidpi_only.framebuffer_to_logical({800.0f, 600.0f}), {800.0f, 600.0f}),
          "dpr alone does not shrink the root size");
    check(near(hidpi_only.window_to_logical({400.0f, 300.0f}), {800.0f, 600.0f}),
          "dpr alone doubles the logical cursor position");
}

// 3) 增减 ui_scale 与 dpr 的组合口径要能自洽：物理 = 逻辑 × ui_scale。
void test_round_trip()
{
    const DisplayScale scale = DisplayScale::make(1.5f, 1.25f);
    const vec2 logical = scale.framebuffer_to_logical({1000.0f, 800.0f});
    const vec2 physical = logical * scale.ui_scale;
    check(near(physical, {1000.0f, 800.0f}), "logical * ui_scale returns the physical size");

    const DisplayScale same = DisplayScale::make(1.5f, 1.25f);
    check(scale == same, "value semantics: equal fields compare equal");
    check(scale != scale.with_dpr(2.0f), "value semantics: dpr change compares unequal");
}

int main()
{
    test_defaults_and_clamp();
    test_conversions();
    test_round_trip();

    std::printf("display scale: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
