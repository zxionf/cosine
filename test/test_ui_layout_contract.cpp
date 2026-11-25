// 阶段 1 安全网（下半）：布局测量契约。
// UI-30 起：本文件是"会失败"的契约测试——任何一条不满足都让 ctest 变红。
// 已知偏差不再用 [KNOWN] 放行；未修复的行为在 test_ui_layout_probe 里以 DEBT 用例显式锁定，
// 修好后由对应任务把断言翻转过来。
#include "graphics/drawable.hpp"
#include "ui/views/file_browser.hpp"
#include "ui/core/label.hpp"
#include "ui/composites/section.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/widget.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace xel;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* name, const char* detail)
{
    if (ok) {
        ++g_passed;
        std::printf("ok      %s\n", name);
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name, detail);
}

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.01f;
}

// 1) 容器在给定宽度约束下不应超出。
void test_measure_respects_constraints()
{
    ui::Vbox box(0.0f, 0.0f, 0.0f, 0.0f);
    box.add(std::make_unique<ui::Label>("child", 18.0f));
    const glm::vec2 size = box.measure({200.0f, 300.0f});
    check(size.x <= 200.0f + 0.01f, "vbox.measure <= constraints.x",
          ("measure.x = " + std::to_string(size.x)).c_str());
}

// 2) 固定尺寸控件会忽略 constraints（fixed_size 声明与行为一致）。
void test_fixed_size_control_ignores_constraints()
{
    ui::FileBrowser browser(glm::vec2{560.0f, 372.0f});
    const glm::vec2 size = browser.measure({100.0f, 100.0f});
    check(near(size.x, 560.0f), "filebrowser keeps its min width",
          ("measure = " + std::to_string(size.x)).c_str());
    check(browser.fixed_size(), "filebrowser declares fixed_size()", "fixed_size() = false");
}

// 3) 子项变化后，容器自己的测量缓存必须失效。
void test_measure_cache_invalidates_on_add()
{
    ui::Vbox box(0.0f, 0.0f, 240.0f, 400.0f);
    box.set_spacing(0.0f);
    const glm::vec2 before = box.measure({240.0f, -1.0f});
    box.add(std::make_unique<ui::Label>("x", 18.0f));
    const glm::vec2 after = box.measure({240.0f, -1.0f});
    check(after.y > before.y + 0.5f, "vbox.measure reflects added child",
          ("before.y = " + std::to_string(before.y) + ", after.y = " + std::to_string(after.y))
              .c_str());
}

// 4) 缓存失效沿 parent 链传播（Section 切页不刷新曾是这个缺陷的根因）。
void test_invalidate_propagates_to_parent()
{
    ui::Vbox parent(0.0f, 0.0f, 300.0f, 400.0f);
    parent.set_spacing(0.0f);
    auto child = std::make_unique<ui::Vbox>(0.0f, 0.0f, 300.0f, 100.0f);
    child->set_spacing(0.0f);
    ui::Vbox* child_ptr = child.get();
    parent.add(std::move(child));

    const glm::vec2 before = parent.measure({300.0f, -1.0f});
    child_ptr->add(std::make_unique<ui::Label>("y", 18.0f));
    const glm::vec2 after = parent.measure({300.0f, -1.0f});
    check(after.y > before.y + 0.5f, "invalidate_measure propagates to parent",
          ("before.y = " + std::to_string(before.y) + ", after.y = " + std::to_string(after.y))
              .c_str());
}

// 5) Section 重建内容后，自身高度要跟着变。
void test_section_rebuild_updates_measure()
{
    ui::Section section("S");
    section.arrange({{0.0f, 0.0f}, {240.0f, 300.0f}});
    const glm::vec2 empty = section.measure({240.0f, -1.0f});
    section.add(std::make_unique<ui::Label>("line", 18.0f));
    const glm::vec2 filled = section.measure({240.0f, -1.0f});
    check(filled.y > empty.y + 0.5f, "section.measure grows after add",
          ("empty.y = " + std::to_string(empty.y) + ", filled.y = " + std::to_string(filled.y))
              .c_str());

    section.clear();
    const glm::vec2 cleared = section.measure({240.0f, -1.0f});
    check(cleared.y < filled.y - 0.5f, "section.measure shrinks after clear",
          ("cleared.y = " + std::to_string(cleared.y)).c_str());
}
} // namespace

int main()
{
    test_measure_respects_constraints();
    test_fixed_size_control_ignores_constraints();
    test_measure_cache_invalidates_on_add();
    test_invalidate_propagates_to_parent();
    test_section_rebuild_updates_measure();

    std::printf("layout contract: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
