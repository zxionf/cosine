// UI-48：部件化就位的契约测试。
//
// R2 只做「字段搬家」（22 个平铺字段 → LayoutParams / BoxMetrics / Interaction / VisualState），
// 行为必须零变化。本测试锁四件事：
//   1. 部件按值内嵌，不是堆分配（sizeof 与指针相等性）；
//   2. Interaction::state() 与旧的自由函数 control_state() 在全部 16 种组合下等价；
//   3. 访问器仍然写/读同一份存储（部件是唯一真相，方法只是转发）；
//   4. 布局部件与测量契约一致（min/max/pct/fill/aspect 的 clamp 行为不变）。
#include "ui/box_metrics.hpp"
#include "ui/core/progress.hpp"
#include "ui/interaction.hpp"
#include "ui/layout_params.hpp"
#include "ui/style.hpp"
#include "ui/visual_state.hpp"
#include "ui/widget.hpp"

#include <cstdio>
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

// 部件是 protected：测试用派生探针暴露只读视图，不改变封装。
struct Exposed : ui::Progress
{
    const ui::LayoutParams& layout_part() const { return layout_; }
    const ui::BoxMetrics& box_part() const { return box; }
    const ui::Interaction& input_part() const { return input; }
};
} // namespace

// 1) 部件是「按值内嵌的小结构」，不是堆分配。
void test_parts_are_inline()
{
    static_assert(sizeof(ui::Interaction) <= 8, "Interaction 必须是 6 个 bool 的小结构");
    static_assert(sizeof(ui::VisualState) == sizeof(void*), "VisualState 目前只持有一个惰性指针");
    static_assert(std::is_trivially_copyable_v<ui::BoxMetrics>, "BoxMetrics 必须是纯数据");

    check(sizeof(ui::Interaction) <= 8, "Interaction fits in 8 bytes");
    check(sizeof(ui::BoxMetrics) == 36, "BoxMetrics is 9 floats (padding/override/margin)");
    check(sizeof(ui::LayoutParams) == 72, "LayoutParams is 18 floats worth of intent");
    // 部件内嵌后 Widget 仍在同一量级（280 → 288，仅对齐损失）。
    check(sizeof(ui::Widget) <= 296, "Widget stays under 296 bytes after part extraction");
}

// 2) 状态映射与旧的自由函数完全等价。
void test_state_mapping_matches_control_state()
{
    int mismatches = 0;
    for (int mask = 0; mask < 16; ++mask) {
        const bool enabled = (mask & 1) != 0;
        const bool hovered = (mask & 2) != 0;
        const bool pressed = (mask & 4) != 0;
        const bool checked = (mask & 8) != 0;

        ui::Interaction state;
        state.enabled = enabled;
        state.hovered = hovered;
        state.pressed = pressed;

        if (state.state(checked) != ui::control_state(enabled, hovered, pressed, checked)) {
            ++mismatches;
        }
    }
    check(mismatches == 0, "Interaction::state() == control_state() for all 16 combinations");

    ui::Interaction disabled;
    disabled.enabled = false;
    disabled.pressed = true;
    disabled.hovered = true;
    check(disabled.state(true) == ui::ControlState::Disabled,
          "Disabled wins over Pressed/Hovered/Checked");
}

// 3) 访问器与部件是同一份存储（不存在第二套真值）。
void test_accessors_share_storage()
{
    Exposed widget;

    widget.set_padding(7.0f);
    check(widget.box_part().padding.left == 7.0f && widget.box_part().padding_override,
          "set_padding writes BoxMetrics and locks the override");
    widget.set_margin(ui::Edges{1.0f, 2.0f, 3.0f, 4.0f});
    check(widget.box_part().margin.left == 1.0f && widget.box_part().margin.bottom == 4.0f,
          "set_margin writes BoxMetrics");

    widget.set_min_size({20.0f, 10.0f});
    widget.set_max_size({200.0f, 100.0f});
    widget.set_fill(true, false);
    widget.set_size_percent(0.5f, 0.25f);
    widget.set_aspect_ratio(2.0f);
    widget.set_flex_grow(3.0f);
    check(widget.layout_part().min_size.x == 20.0f && widget.layout_part().max_size.y == 100.0f,
          "min/max size live in LayoutParams");
    check(widget.layout_part().fill_x && !widget.layout_part().fill_y, "fill lives in LayoutParams");
    check(widget.layout_part().size_percent.x == 0.5f && widget.layout_part().aspect_ratio == 2.0f,
          "pct/aspect live in LayoutParams");
    check(widget.layout_part().flex_grow == 3.0f, "flex_grow lives in LayoutParams");

    widget.set_hovered(true);
    widget.set_focused(true);
    check(widget.input_part().hovered && widget.input_part().focused,
          "hovered/focused live in Interaction");
    check(widget.is_hovered() && widget.is_focused(), "accessors read Interaction back");

    widget.on_focus_lost();
    check(!widget.input_part().focused, "on_focus_lost clears Interaction::focused");
}

// 4) 布局部件的派生量仍与布局契约一致。
void test_layout_part_derived_values()
{
    Exposed widget;
    widget.set_min_size({30.0f, 12.0f});
    widget.set_max_size({90.0f, 40.0f});

    const glm::vec2 clamped = widget.clamp_size({500.0f, 500.0f});
    check(clamped.x == 90.0f && clamped.y == 40.0f, "clamp_size uses LayoutParams min/max");

    widget.set_preferred_size({60.0f, 20.0f});
    const auto spec = widget.layout_spec();
    check(spec.min_size.x == 30.0f && spec.max_size.y == 40.0f,
          "layout_spec() snapshots LayoutParams");

    widget.set_display(ui::Widget::Display::Hidden);
    check(widget.display() == ui::Widget::Display::Hidden && !widget.visible(),
          "Hidden display implies not visible");
    widget.set_display(ui::Widget::Display::Flow);
    check(widget.display() == ui::Widget::Display::Flow && widget.visible(),
          "Flow display implies visible");
    check(widget.layout_part().display == ui::Display::Flow,
          "Widget::Display aliases ui::Display (single definition)");
}

int main()
{
    test_parts_are_inline();
    test_state_mapping_matches_control_state();
    test_accessors_share_storage();
    test_layout_part_derived_values();

    std::printf("state ownership: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
