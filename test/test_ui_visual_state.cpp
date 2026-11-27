// UI-50：样式过渡索引化的契约测试。
//
// 三条不变量（.design/ui-redesign.md K5 + docs/todo-ui.md R4）：
//   1. 槽只存「状态对 + 进度」，颜色渲染时从 Theme 现算：8 B/槽、整块 ≤128 B、内嵌无堆；
//   2. 稳定态输出与 Theme::style(role, state) 逐字段一致（12 角色 × 5 状态）；
//   3. 过渡语义不变：首帧吸附、中途反向不跳变、主题 revision 变化时逐槽重建；
//      并锁住 UI-46 的教训——同一帧同一 role 只能一种状态（不同 role 互不影响）。
#include "ui/core/button.hpp"
#include "ui/core/slider.hpp"
#include "ui/style/style.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/ui_system.hpp"
#include "ui/style/visual_state.hpp"
#include "ui/style/widget_style.hpp"

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

bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

bool near_color(glm::vec4 a, glm::vec4 b, float eps = 1e-4f)
{
    return near(a.r, b.r, eps) && near(a.g, b.g, eps) && near(a.b, b.b, eps) && near(a.a, b.a, eps);
}

const ui::ControlState kStates[] = {ui::ControlState::Normal, ui::ControlState::Hovered,
                                    ui::ControlState::Pressed, ui::ControlState::Disabled,
                                    ui::ControlState::Checked};

const ui::StyleRole kRoles[] = {
    ui::StyleRole::Surface,  ui::StyleRole::Control,  ui::StyleRole::Input,
    ui::StyleRole::Track,    ui::StyleRole::Accent,   ui::StyleRole::Row,
    ui::StyleRole::MenuItem, ui::StyleRole::Tooltip,  ui::StyleRole::Chrome,
    ui::StyleRole::RowSelected, ui::StyleRole::Knob,  ui::StyleRole::Thumb,
};

struct Fixture
{
    ui::Button* button = nullptr;
    std::unique_ptr<ui::UISystem> system;

    explicit Fixture(float transition_ms)
    {
        auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto btn = std::make_unique<ui::Button>("ok");
        button = btn.get();
        root->add(std::move(btn));
        system = std::make_unique<ui::UISystem>(std::move(root));
        system->tick();
        system->set_theme(ui::Theme::dark().with_transition_ms(transition_ms));
        button->arrange({{0.0f, 0.0f}, {120.0f, 30.0f}});
    }
    const ui::Theme& theme() const { return system->context()->theme(); }
    ui::BoxStyle style(ui::StyleRole role, ui::ControlState state) const
    {
        return button->visual_style(role, state);
    }
};
} // namespace

// 1) 内存：8 B/槽、整块 ≤128 B、可平凡析构（没有 unique_ptr → 不再堆分配）。
void test_slot_is_compact()
{
    static_assert(sizeof(ui::StyleSlot) == 8, "StyleSlot 必须是 8 B（状态对 + 进度）");
    static_assert(sizeof(ui::VisualState) <= 128, "VisualState 必须 ≤128 B");
    static_assert(std::is_trivially_destructible_v<ui::VisualState>,
                  "VisualState 不得持有堆对象（UI-50 去堆分配）");

    check(sizeof(ui::StyleSlot) == 8, "StyleSlot is 8 bytes");
    check(sizeof(ui::VisualState) <= 128, "VisualState fits in 128 bytes");
    check(std::is_trivially_destructible_v<ui::VisualState>, "VisualState has no heap ownership");
}

// 2) 稳定态：12 角色 × 5 状态，输出与 Theme::style() 逐字段一致。
void test_settled_style_matches_theme()
{
    Fixture f(0.0f);
    int mismatches = 0;
    for (auto role : kRoles) {
        for (auto state : kStates) {
            const ui::BoxStyle expected = f.theme().style(role, state);
            const ui::BoxStyle actual = f.style(role, state);
            if (!near_color(actual.fill, expected.fill) || !near_color(actual.border, expected.border)
                || !near_color(actual.text, expected.text)
                || !near(actual.border_width, expected.border_width)
                || !near(actual.radius, expected.radius) || !near(actual.text_size, expected.text_size)) {
                ++mismatches;
            }
        }
    }
    check(mismatches == 0, "settled slot equals Theme::style() for 12 roles x 5 states");
}

// 3) 过渡语义：首帧吸附、中间插值、补满到位、中途反向不跳变。
void test_transition_semantics()
{
    Fixture f(100.0f);
    const auto& theme = f.theme();

    // 首帧吸附（不做淡入）。
    check(near_color(f.style(ui::StyleRole::Control, ui::ControlState::Normal).fill, theme.button_bg),
          "first use snaps to Normal");

    f.button->set_hovered(true);
    check(near_color(f.style(ui::StyleRole::Control, ui::ControlState::Hovered).fill, theme.button_bg),
          "state change starts from the previous state (no jump)");

    f.system->tick(0.05f);
    const auto mid = f.style(ui::StyleRole::Control, ui::ControlState::Hovered);
    check(mid.fill.r > theme.button_bg.r && mid.fill.r < theme.button_hover.r,
          "mid transition is strictly between the two colors");

    // 反向：hover -> normal，进度往回走，颜色应落在 bg..hover 之间且朝 bg 移动。
    f.button->set_hovered(false);
    const auto reversed = f.style(ui::StyleRole::Control, ui::ControlState::Normal);
    check(reversed.fill.r > theme.button_bg.r && reversed.fill.r < theme.button_hover.r,
          "reverse keeps continuity (does not jump to either end)");

    f.system->tick(0.05f);
    const auto back = f.style(ui::StyleRole::Control, ui::ControlState::Normal);
    check(back.fill.r < reversed.fill.r, "reverse moves back toward the starting color");

    f.system->tick(0.5f);
    check(near_color(f.style(ui::StyleRole::Control, ui::ControlState::Normal).fill, theme.button_bg),
          "reverse settles back at the starting color");
    check(!f.button->transition_active(), "transition is inactive after settling");
}

// 4) 一帧一角色一状态（UI-46 教训）：不同 role 互不影响，同 role 同帧只有最后一种状态。
void test_role_slots_are_independent()
{
    Fixture f(0.0f);
    const auto track = f.style(ui::StyleRole::Track, ui::ControlState::Normal);
    const auto thumb = f.style(ui::StyleRole::Thumb, ui::ControlState::Normal);
    const auto accent = f.style(ui::StyleRole::Accent, ui::ControlState::Normal);

    check(!near_color(track.fill, thumb.fill), "Track and Thumb resolve to different colors");
    check(!near_color(track.fill, accent.fill), "Track and Accent resolve to different colors");
    check(near_color(f.theme().style(ui::StyleRole::Track, ui::ControlState::Normal).fill, track.fill),
          "Track slot keeps its own state (no cross-role contamination)");
}

// 5) 主题变化：revision 推进 → 逐槽重建，不做过渡。
void test_theme_change_resets_slots()
{
    Fixture f(100.0f);
    f.button->set_hovered(true);
    f.style(ui::StyleRole::Control, ui::ControlState::Hovered);
    f.system->tick(0.02f); // 过渡进行中

    const ui::Theme light = ui::Theme::light().with_transition_ms(100.0f);
    f.system->set_theme(light);

    const auto after = f.style(ui::StyleRole::Control, ui::ControlState::Hovered);
    check(near_color(after.fill, light.style(ui::StyleRole::Control, ui::ControlState::Hovered).fill),
          "theme change rebuilds slots to the new theme without transition");
    check(!f.button->transition_active(), "no transition is left running after theme change");
}

int main()
{
    test_slot_is_compact();
    test_settled_style_matches_theme();
    test_transition_semantics();
    test_role_slots_are_independent();
    test_theme_change_resets_slots();

    std::printf("visual state: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
