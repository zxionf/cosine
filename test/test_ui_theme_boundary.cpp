// UI-42：主题可变性边界闭合。
//
// 三条不变量：
//   1. 主题只能经 UIContext::set_theme() 修改，它会推进本 context 的 theme_revision；
//   2. revision 挂在 context 上（不是进程全局）——多个 root 交替 tick 不互相打断样式过渡缓存；
//   3. 依赖主题的派生量（样式过渡、测量）在 set_theme 后立刻反映新值。
#include "graphics/drawable.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/root.hpp"
#include "ui/theme.hpp"
#include "ui/ui_system.hpp"

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

bool near_color(glm::vec4 a, glm::vec4 b, float eps = 0.01f)
{
    return std::abs(a.r - b.r) < eps && std::abs(a.g - b.g) < eps
        && std::abs(a.b - b.b) < eps;
}

// 一个 root + 一个按钮的最小装置。
struct Rig
{
    std::unique_ptr<ui::UISystem> system;
    ui::Button* button = nullptr;
    ui::Label* label = nullptr;

    ui::UIContext* ctx() const { return system->context(); }

    explicit Rig(const ui::Theme& theme)
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, 400.0f, 300.0f);
        box->set_padding(0.0f);
        auto btn = std::make_unique<ui::Button>("action");
        button = btn.get();
        box->add(std::move(btn));
        auto lbl = std::make_unique<ui::Label>("text", 16.0f);
        label = lbl.get();
        box->add(std::move(lbl));
        root->add(std::move(box));
        system = std::make_unique<ui::UISystem>(std::move(root));
        system->set_theme(theme);
        system->tick();
    }
};

// 主题版本：set_theme 推进，读 theme() 不推进。
void test_revision_advances_on_set_theme()
{
    Rig rig(ui::Theme::dark());
    const auto before = rig.ctx()->theme_revision();
    rig.ctx()->set_theme(ui::Theme::light());
    const auto after = rig.ctx()->theme_revision();
    check(after > before, "set_theme advances the context revision",
          std::to_string(before) + " -> " + std::to_string(after));

    // 只读访问不改版本。
    (void)rig.ctx()->theme();
    check(rig.ctx()->theme_revision() == after, "reading theme() does not bump revision");
}

// 两个 root 各持一套主题：交替 tick 后各自保持自己的主题（不串色）。
void test_two_roots_keep_own_theme()
{
    Rig dark_rig(ui::Theme::dark());
    Rig light_rig(ui::Theme::light());

    for (int i = 0; i < 5; ++i) {
        dark_rig.system->tick();
        light_rig.system->tick();
    }

    const auto dark_style = dark_rig.button->visual_style(ui::StyleRole::Control,
                                                         ui::ControlState::Normal);
    const auto light_style = light_rig.button->visual_style(ui::StyleRole::Control,
                                                           ui::ControlState::Normal);
    const auto& dark_theme = dark_rig.ctx()->theme();
    const auto& light_theme = light_rig.ctx()->theme();

    check(near_color(dark_style.fill, dark_theme.button_bg),
          "dark root keeps the dark palette", "fill mismatch");
    check(near_color(light_style.fill, light_theme.button_bg),
          "light root keeps the light palette", "fill mismatch");
    check(!near_color(dark_style.fill, light_style.fill),
          "the two roots do not share one palette");

    // 各自的 revision 独立推进。
    const auto dark_rev = dark_rig.ctx()->theme_revision();
    light_rig.ctx()->set_theme(ui::Theme::light().with_density(ui::Density::Compact));
    check(dark_rig.ctx()->theme_revision() == dark_rev,
          "changing root B does not touch root A's revision",
          std::to_string(dark_rev) + " -> " + std::to_string(dark_rig.ctx()->theme_revision()));
}

// set_theme 后派生量立即更新（不需要等一次 hover）。
void test_set_theme_refreshes_derived_state()
{
    Rig rig(ui::Theme::dark());
    const auto before = rig.button->visual_style(ui::StyleRole::Control, ui::ControlState::Normal);

    ui::Theme light = ui::Theme::light();
    light.control_height = 44.0f;
    rig.system->set_theme(light);
    rig.system->tick();

    const auto after = rig.button->visual_style(ui::StyleRole::Control, ui::ControlState::Normal);
    check(!near_color(before.fill, after.fill), "button fill follows the new theme immediately");
    check(rig.button->height() > 30.0f, "control_height from the new theme applied",
          std::to_string(rig.button->height()));
    check(near_color(after.fill, light.button_bg), "resolved style matches the new palette");
}

// 控件没有 context 时回退到默认主题，且不崩。
void test_no_context_fallback()
{
    ui::Button standalone("solo");
    standalone.arrange({{0.0f, 0.0f}, {80.0f, 30.0f}});
    const auto style = standalone.visual_style(ui::StyleRole::Control, ui::ControlState::Normal);
    check(near_color(style.fill, ui::theme().button_bg),
          "widget without context falls back to the default theme");
}
} // namespace

int main()
{
    test_revision_advances_on_set_theme();
    test_two_roots_keep_own_theme();
    test_set_theme_refreshes_derived_state();
    test_no_context_fallback();

    std::printf("theme boundary: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
