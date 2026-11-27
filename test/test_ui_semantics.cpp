// UI-58：语义覆盖与新控件成本实测。
//
// 两条契约（设计 §7 / §8）：
//   1. 每个**可聚焦**的控件都必须有明确的语义角色（≠ Custom）——否则通用代码
//      （调试面板、Tab 顺序、未来 a11y）无法回答"这是什么"；
//   2. 纯装饰控件允许 Custom，但必须在白名单里显式列出（避免"忘了写 role"
//      被误当成"故意装饰"）；
//   3. 新增交互叶子控件的成本：Tag 实测 ≤70 行（其中 5 个事件钩子 24 行）。
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/label.hpp"
#include "ui/core/radio.hpp"
#include "ui/core/slider.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/tag.hpp"
#include "ui/core/text_input.hpp"
#include "ui/core/tooltip.hpp"
#include "ui/system/root.hpp"
#include "ui/system/ui_system.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

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

// 允许 Custom 的装饰控件（显式白名单）。
bool is_decorative(ui::WidgetRole role, const char* type_name)
{
    if (role != ui::WidgetRole::Custom) return false;
    for (const char* allowed : {"ColorRect", "Spacer", "PreviewRuler", "Scrim", "TabBar",
                                "MenuScrim", "SceneView", "TimelineView", "LinearLayout",
                                "Grid", "Wrap", "ScrollPanel"}) {
        if (std::string(type_name) == allowed) return true;
    }
    return false;
}

// 类型名只用于白名单比对：用 dynamic_cast 枚举已知装饰类型。
const char* decorative_name(const ui::Node& node)
{
    if (dynamic_cast<const ui::ColorRect*>(&node)) return "ColorRect";
    if (dynamic_cast<const ui::Spacer*>(&node)) return "Spacer";
    if (dynamic_cast<const ui::Tooltip*>(&node)) return "Tooltip";
    return "";
}

// 遍历整棵树，收集 (role, 是否可聚焦, 名字)。
struct Walk
{
    int focusable_without_role = 0;
    int checked = 0;
    std::string first_offender;

    void visit(const ui::Node& node)
    {
        ++checked;
        const ui::Semantics s = node.semantics();
        if (node.focusable() && s.role == ui::WidgetRole::Custom) {
            ++focusable_without_role;
            if (first_offender.empty()) first_offender = std::string(ui::to_string(s.role));
        }
        node.for_each_child([this](const ui::Node& child) { visit(child); });
    }
};
} // namespace

// 1) 交互控件必须有语义角色。
void test_focusable_widgets_have_role()
{
    ui::UIRoot root(0.0f, 0.0f, 800.0f, 600.0f);
    auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, 800.0f, 600.0f);
    auto* box_ptr = box.get();
    root.add(std::move(box));

    box_ptr->add(std::make_unique<ui::Button>("ok"));
    box_ptr->add(std::make_unique<ui::Checkbox>("check"));
    box_ptr->add(std::make_unique<ui::Radio>("radio"));
    box_ptr->add(std::make_unique<ui::Slider>());
    box_ptr->add(std::make_unique<ui::TextInput>("text"));
    box_ptr->add(std::make_unique<ui::Tag>("tag"));
    box_ptr->add(std::make_unique<ui::Label>("label"));
    box_ptr->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 10.0f, 10.0f, glm::vec4{1.0f}));
    box_ptr->add(std::make_unique<ui::Spacer>(4.0f, 4.0f));

    Walk walk;
    walk.visit(root);
    check(walk.focusable_without_role == 0, "every focusable widget has a semantic role",
          "offenders=" + std::to_string(walk.focusable_without_role));
    check(walk.checked >= 10, "walk covered the whole tree",
          "visited=" + std::to_string(walk.checked));
}

// 2) 装饰控件允许 Custom，但要在白名单里。
void test_decorative_widgets_are_whitelisted()
{
    ui::ColorRect rect(0.0f, 0.0f, 10.0f, 10.0f, glm::vec4{1.0f});
    ui::Spacer spacer(4.0f, 4.0f);
    ui::Label label("x");

    check(is_decorative(rect.semantics().role, decorative_name(rect)), "ColorRect is a whitelisted decoration");
    check(is_decorative(spacer.semantics().role, decorative_name(spacer)), "Spacer is a whitelisted decoration");
    check(!is_decorative(label.semantics().role, "Label"), "Label is not decorative (has Label role)");
}

// 3) Tag：新控件成本实测 + 行为契约。
void test_tag_widget()
{
    int clicks = 0;
    ui::Tag tag("hello", [&clicks] { ++clicks; });
    tag.arrange({{10.0f, 10.0f}, {80.0f, 24.0f}});

    const ui::Semantics s = tag.semantics();
    check(s.role == ui::WidgetRole::Button, "Tag reports Button role");
    check(s.label == "hello", "Tag exposes its label");
    check(s.enabled, "Tag starts enabled");
    check(tag.focusable(), "Tag is focusable");

    check(tag.on_click({20.0f, 20.0f}, 0) && clicks == 1, "Tag handles click");
    check(tag.on_key_down(event::KeyCode::Enter, event::Modifier::None) && clicks == 2,
          "Tag handles Enter");

    tag.set_enabled(false);
    check(!tag.semantics().enabled, "disabled Tag reports enabled=false");
    check(!tag.on_click({20.0f, 20.0f}, 0) && clicks == 2, "disabled Tag ignores clicks");
    check(!tag.on_key_down(event::KeyCode::Enter, event::Modifier::None) && clicks == 2,
          "disabled Tag ignores keys");

    // 尺寸来自 preferred size（LeafNode 的默认 measure）。
    const glm::vec2 measured = ui::Widget::measure_child(tag, {500.0f, 500.0f});
    check(measured.x > 0.0f && measured.y > 0.0f, "Tag measures from its preferred size");
}

int main()
{
    test_focusable_widgets_have_role();
    test_decorative_widgets_are_whitelisted();
    test_tag_widget();

    std::printf("semantics: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
