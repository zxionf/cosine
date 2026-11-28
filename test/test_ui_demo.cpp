// ui_demo 的结构契约测试：展示台本身不需要窗口/Vulkan 就能验证。
//
// 覆盖：
//   1. 注册表：id 唯一、每条都能构建、标题非空；
//   2. 外壳：顶栏/侧栏/内容/状态栏都在树上；
//   3. 分区切换：懒构建只建一次，隐藏的页不参与布局；
//   4. 主题开关：改主题后 context revision 推进（样式过渡会重建）；
//   5. 状态栏：update() 后文本包含当前分区名与指标。
#include "demo.hpp"

#include "ui/system/ui_system.hpp"

#include <cstdio>
#include <set>
#include <string>

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

bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

struct Fixture
{
    std::unique_ptr<ui::UIRoot> root{std::make_unique<ui::UIRoot>(0.0f, 0.0f, 1200.0f, 800.0f)};
    ui::UISystem ui{std::move(root)};
    std::unique_ptr<demo::Demo> demo;

    Fixture() { demo = demo::create(*ui.root(), {}); }
};
} // namespace

// 1) 注册表本身：可扩展性的前提是每条记录都完整。
void test_section_registry()
{
    const auto& list = demo::section_list();
    check(!list.empty(), "registry is not empty");

    std::set<std::string> ids;
    int incomplete = 0;
    for (const auto& section : list) {
        if (section.id.empty() || section.title.empty() || section.build == nullptr) ++incomplete;
        ids.insert(section.id);
    }
    check(incomplete == 0, "every section has id/title/build",
          "incomplete=" + std::to_string(incomplete));
    check(ids.size() == list.size(), "section ids are unique",
          "unique=" + std::to_string(ids.size()) + " total=" + std::to_string(list.size()));
}

// 2) 外壳：顶栏 / 标签页 / 状态栏都在树上，且首屏已经有内容页。
void test_shell_is_built()
{
    Fixture fixture;
    check(fixture.demo != nullptr, "demo instance exists");
    check(fixture.demo->shell().tabs != nullptr, "shell has a tab view (second row)");
    check(fixture.demo->shell().status != nullptr, "shell has a status label");
    check(fixture.demo->current_index() == 0, "first section is shown by default");
    check(fixture.ui.root()->child_count() == 1, "shell is attached as the single root child");

    // 标签数 == 分区数：导航就是标签页本身。
    const auto& list = demo::section_list();
    check(fixture.demo->shell().tabs->page_count() == static_cast<int>(list.size()),
          "one tab per section",
          "tabs=" + std::to_string(fixture.demo->shell().tabs->page_count()));
}

// 3) 分区切换：标签页驱动的懒构建；未打开的标签页不建内容。
void test_section_switching_is_lazy()
{
    Fixture fixture;
    const auto& list = demo::section_list();
    check(fixture.demo->built_page_count() == 1, "only the first section is built up front",
          "built=" + std::to_string(fixture.demo->built_page_count()));

    for (std::size_t i = 1; i < list.size(); ++i) {
        fixture.demo->show_section(static_cast<int>(i));
        check(fixture.demo->current_index() == static_cast<int>(i),
              "show_section(" + list[i].id + ") activates it");
    }
    check(fixture.demo->built_page_count() == static_cast<int>(list.size()),
          "each visited tab is built exactly once",
          "built=" + std::to_string(fixture.demo->built_page_count()));

    // 回到第一个分区不会重复构建。
    const int built = fixture.demo->built_page_count();
    fixture.demo->show_section(0);
    check(fixture.demo->built_page_count() == built, "switching back does not rebuild pages");

    // 页签选中联动：selected() 跟随 show_section。
    check(fixture.demo->shell().tabs->selected() == 0, "tab bar follows show_section");
    fixture.demo->show_section(3);
    check(fixture.demo->shell().tabs->selected() == 3, "tab bar selects the active section");
}

// 4) 主题开关：revision 必须推进，否则样式过渡缓存不会重建。
void test_theme_switches()
{
    Fixture fixture;
    auto* context = fixture.ui.context();
    check(context != nullptr, "context exists");
    if (context == nullptr) return;

    const std::uint64_t before = context->theme_revision();
    fixture.demo->toggle_dark();
    check(!fixture.demo->dark(), "toggle_dark flips to light");
    check(context->theme_revision() > before, "theme revision advances on switch");

    fixture.demo->cycle_density();
    fixture.demo->cycle_focus_policy();
    fixture.demo->toggle_rounded();
    fixture.demo->toggle_transitions();
    check(context->theme_revision() > before, "every theme tweak goes through set_theme");

    const auto& scale = fixture.demo->ui_scale();
    check(scale == 1.0f, "default ui scale is 1.0");
    fixture.demo->set_ui_scale(1.25f); // 未注入回调时应给出提示而不是崩溃
    check(fixture.demo->ui_scale() == 1.0f, "scale is unchanged without a host hook");
}

// 5) 状态栏：update() 后带当前分区名与运行指标。
void test_status_line()
{
    Fixture fixture;
    graphics::FrameStats stats;
    stats.fps = 60.0;
    stats.frame_time_ms = 16.7;
    stats.draw.draw_calls = 7;
    stats.draw.vertices = 1234;

    fixture.demo->show_section(1);
    fixture.demo->update(1.0f / 60.0f, stats, nullptr, nullptr);

    const std::string text{fixture.demo->shell().status->semantics_label()};
    check(contains(text, demo::section_list()[1].title), "status shows the active section", text);
    check(contains(text, "fps"), "status shows fps", text);
    check(contains(text, "draw 7"), "status shows draw calls", text);

    fixture.demo->note("自定义反馈");
    fixture.demo->update(1.0f / 60.0f, stats, nullptr, nullptr);
    check(contains(std::string{fixture.demo->shell().status->semantics_label()}, "自定义反馈"),
          "note() is surfaced in the status bar");
}

int main()
{
    test_section_registry();
    test_shell_is_built();
    test_section_switching_is_lazy();
    test_theme_switches();
    test_status_line();

    std::printf("ui demo: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
