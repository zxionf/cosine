// UI-34：主题 / 密度 / 字号切换后的度量一致性。
//
// 目标：切换 theme 之后，容器的 measure 与内容保持一致，且多次切换回到原档位时尺寸可回到原值
// （不漂移、不缓存旧度量）。
#include "graphics/drawable.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/system/root.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/ui_system.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace xel;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* name, const std::string& detail = std::string())
{
    if (ok) {
        ++g_passed;
        std::printf("ok      %s\n", name);
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name, detail.c_str());
}

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.51f; // 布局取整容差
}

// 嵌套结构：Vbox > [Button, ScrollPanel > Vbox > Label x2]
struct Fixture
{
    std::unique_ptr<ui::UIRoot> root;
    ui::Vbox* outer = nullptr;
    ui::Button* button = nullptr;
    ui::ScrollPanel* panel = nullptr;
    ui::Label* label = nullptr;
};

Fixture build()
{
    Fixture f;
    f.root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 480.0f, 360.0f);
    auto outer = std::make_unique<ui::Vbox>(0.0f, 0.0f, 480.0f, 360.0f);
    outer->set_padding(0.0f);
    outer->set_spacing(0.0f);
    f.outer = outer.get();

    auto button = std::make_unique<ui::Button>("action");
    f.button = button.get();
    outer->add(std::move(button));

    auto panel = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 200.0f);
    panel->set_padding(0.0f);
    f.panel = panel.get();
    auto label = std::make_unique<ui::Label>("nested label", 16.0f);
    f.label = label.get();
    panel->add(std::move(label));
    outer->add(std::move(panel));

    f.root->add(std::move(outer));
    f.root->layout();
    return f;
}

void test_density_cycle_is_stable()
{
    Fixture f = build();
    const float base_outer = f.outer->measure({480.0f, 360.0f}).y;
    const float base_button = f.button->measure({480.0f, 360.0f}).y;
    const float base_label = f.label->measure({480.0f, 360.0f}).y;

    // 依次切到 Compact / Spacious / Normal，再检查尺寸。
    for (const ui::Density d : {ui::Density::Compact, ui::Density::Spacious, ui::Density::Normal}) {
        f.root->set_theme(f.root->ui_theme().with_density(d));
        f.root->layout();
    }

    const float back_outer = f.outer->measure({480.0f, 360.0f}).y;
    const float back_button = f.button->measure({480.0f, 360.0f}).y;
    const float back_label = f.label->measure({480.0f, 360.0f}).y;
    check(near(back_outer, base_outer) && near(back_button, base_button)
              && near(back_label, base_label),
          "density cycle returns to the original metrics",
          std::to_string(base_outer) + " -> " + std::to_string(back_outer));

    // 容器高度 = 子项高度之和（button + panel 自然高度）。
    const float panel_h = f.panel->measure({480.0f, 360.0f}).y;
    check(near(f.outer->measure({480.0f, 360.0f}).y, f.button->measure({480.0f, 360.0f}).y + panel_h),
          "outer.measure == button + panel",
          std::to_string(f.outer->measure({480.0f, 360.0f}).y) + " vs "
              + std::to_string(f.button->measure({480.0f, 360.0f}).y + panel_h));
}

void test_density_changes_metrics()
{
    Fixture f = build();
    const float normal_button = f.button->measure({480.0f, 360.0f}).y;

    ui::Theme compact = f.root->ui_theme().with_density(ui::Density::Compact);
    f.root->set_theme(compact);
    f.root->layout();
    const float compact_button = f.button->measure({480.0f, 360.0f}).y;

    f.root->set_theme(f.root->ui_theme().with_density(ui::Density::Spacious));
    f.root->layout();
    const float spacious_button = f.button->measure({480.0f, 360.0f}).y;

    check(compact_button < normal_button && normal_button < spacious_button,
          "button height follows density (compact < normal < spacious)",
          std::to_string(compact_button) + " / " + std::to_string(normal_button) + " / "
              + std::to_string(spacious_button));
}

void test_text_size_change_propagates()
{
    Fixture f = build();
    const float before = f.outer->measure({480.0f, 360.0f}).y;

    ui::Theme bigger = f.root->ui_theme();
    bigger.text_size = 24.0f;
    bigger.small_text_size = 18.0f;
    bigger.heading_text_size = 32.0f;
    bigger.control_height = 40.0f;
    f.root->set_theme(bigger);
    f.root->layout();
    const float after = f.outer->measure({480.0f, 360.0f}).y;

    check(after > before + 1.0f, "text size change grows the nested container",
          std::to_string(before) + " -> " + std::to_string(after));
    // arrange 之后子项落在父容器内。
    check(f.panel->global_bounds().pos.y + f.panel->global_bounds().size.y
              <= f.outer->global_bounds().pos.y + f.outer->global_bounds().size.y + 0.51f,
          "nested panel stays inside outer after text resize");
}

void test_padding_override_survives_theme()
{
    Fixture f = build();
    check(near(f.panel->padding().left, 0.0f), "panel padding override starts at 0",
          std::to_string(f.panel->padding().left));
    f.root->set_theme(f.root->ui_theme().with_density(ui::Density::Spacious));
    f.root->layout();
    check(near(f.panel->padding().left, 0.0f), "explicit set_padding(0) survives theme refresh",
          std::to_string(f.panel->padding().left));
    check(near(f.outer->padding().left, 0.0f), "bare Vbox explicit padding survives",
          std::to_string(f.outer->padding().left));
}

void test_light_theme_keeps_geometry()
{
    Fixture f = build();
    const glm::vec2 normal = f.outer->measure({480.0f, 360.0f});
    f.root->set_theme(ui::Theme::light());
    f.root->layout();
    const glm::vec2 light = f.outer->measure({480.0f, 360.0f});
    check(near(normal.x, light.x) && near(normal.y, light.y),
          "light theme keeps the same geometry", std::to_string(light.y));
}

void test_measure_valid_after_layout()
{
    Fixture f = build();
    f.root->layout();
    check(f.outer->measure_valid() && f.button->measure_valid(),
          "layout leaves measure caches valid (early-out invariant)");
    // 校验早退后仍能失效：改一下按钮尺寸，outer 必须重算。
    const float before = f.outer->measure({480.0f, 360.0f}).y;
    f.button->set_min_size({0.0f, 80.0f});
    const float after = f.outer->measure({480.0f, 360.0f}).y;
    check(after >= before + 1.0f, "child change invalidates the parent measure after layout",
          std::to_string(before) + " -> " + std::to_string(after));
}
} // namespace

int main()
{
    test_density_cycle_is_stable();
    test_density_changes_metrics();
    test_text_size_change_propagates();
    test_padding_override_survives_theme();
    test_light_theme_keeps_geometry();
    test_measure_valid_after_layout();

    std::printf("theme relayout: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
