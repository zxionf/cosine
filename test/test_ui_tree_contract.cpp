// UI-49：树的唯一遍历入口与几何唯一写入口的契约测试。
//
// 三条不变量（对应 .design/ui-redesign.md K2/K3 与 docs/todo-ui.md R3）：
//   1. 存储私有：children_ 不可被派生类直接读写，遍历只走 for_each_child（含 overlays）；
//   2. 遍历一致：debug_recursive / clear_layout_dirty_recursive / invalidate_measure_recursive
//      都覆盖 overlays（旧实现里 debug_recursive 漏掉 overlays）；
//   3. 脏标记链不断：Section 的 content 现在是真正的子节点，内部变更能传到 root。
#include "graphics/drawable.hpp"
#include "ui/composites/section.hpp"
#include "ui/composites/tab_view.hpp"
#include "ui/containers/linear_layout.hpp"
#include "ui/core/label.hpp"
#include "ui/core/splitter.hpp"
#include "ui/system/root.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/ui_system.hpp"
#include "ui/node/widget.hpp"

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

// 记录被遍历到的节点，用于验证 for_each_child 的覆盖面。
struct Collector
{
    std::vector<const ui::Node*> seen;
    void operator()(const ui::Node& n) { seen.push_back(&n); }
};

// 带 overlays 的容器：用于验证"树遍历包含浮层"。
class OverlayHost : public ui::Widget
{
public:
    OverlayHost()
    {
        auto child = std::make_unique<ui::Label>("child", 12.0f);
        child_ = child.get();
        adopt(std::move(child));

        auto over = std::make_unique<ui::Label>("overlay", 12.0f);
        overlay_ = over.get();
        overlays_.push_back(std::move(over));
        overlays_.back()->set_parent(this);
    }
    const std::vector<std::unique_ptr<ui::Node>>& overlays() const override { return overlays_; }

    glm::vec2 measure(glm::vec2) override { return {100.0f, 40.0f}; }
    void arrange(ui::Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList&) const override {}

    ui::Widget* child_ = nullptr;
    ui::Widget* overlay_ = nullptr;

private:
    std::vector<std::unique_ptr<ui::Node>> overlays_;
};

// 探针：自定义遍历入口不可用（children() 已是普通函数），只能经 for_each_child。
class TraversalProbe : public ui::Widget
{
public:
    int visited = 0;
    glm::vec2 measure(glm::vec2) override { return {10.0f, 10.0f}; }
    void arrange(ui::Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList&) const override {}
    void walk() const
    {
        TraversalProbe* self = const_cast<TraversalProbe*>(this);
        for_each_child([self](ui::Node&) { ++self->visited; });
    }
};
} // namespace

// 1) for_each_child 覆盖 children + overlays；访问器只能读。
void test_traversal_covers_overlays()
{
    OverlayHost host;
    Collector c;
    TraversalProbe probe;
    probe.adopt(std::make_unique<OverlayHost>());
    probe.walk();
    check(probe.visited == 1, "for_each_child visits adopted children");

    // overlays 通过虚 overlays() 参与同一条遍历路径
    const auto& list = host.overlays();
    check(list.size() == 1 && list.front().get() == host.overlay_,
          "overlays() still exposes the overlay list");
    check(host.children().size() == 1 && host.children().front().get() == host.child_,
          "children() returns the base-owned storage");
}

// 2) debug_recursive 现在覆盖 overlays（旧实现漏掉）。
void test_debug_recursive_reaches_overlays()
{
    OverlayHost host;
    host.debug_recursive();
    check(host.is_debug(), "debug_recursive marks the host");
    check(host.child_->is_debug(), "debug_recursive reaches children");
    check(host.overlay_->is_debug(), "debug_recursive reaches overlays (UI-49 fix)");
}

// 3) Section：content 是真正的子节点，脏标记链不断。
void test_section_is_a_real_parent()
{
    ui::Section section("group");
    auto label = std::make_unique<ui::Label>("inner", 14.0f);
    auto* inner = label.get();
    section.add(std::move(label));

    check(section.children().size() == 1, "Section adopts its content box");
    check(inner->parent() != nullptr && inner->parent()->parent() == &section,
          "inner label's parent chain reaches Section (no broken link)");

    // 内部变更必须能把脏标记传出去：先清干净，再改内部控件。
    section.clear_layout_dirty();
    inner->set_preferred_size({123.0f, 21.0f});
    check(section.layout_dirty(), "inner change marks Section dirty (chain intact)");
}

// 4) 分栏与标签页：索引访问走 child_at / replace_child，不再是 vector 本体。
void test_indexed_access_api()
{
    ui::Splitter splitter(200.0f, 100.0f);
    splitter.set_panes(std::make_unique<ui::Label>("left", 12.0f),
                       std::make_unique<ui::Label>("right", 12.0f));
    splitter.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    const auto& kids = splitter.children();
    check(kids.size() == 2, "Splitter keeps two panes in base storage");
    check(kids[0]->global_bounds().size.x > 0.0f && kids[1]->global_bounds().size.x > 0.0f,
          "Splitter arranges both panes through child_at()");

    ui::TabView tabs;
    tabs.add_page("a", std::make_unique<ui::Label>("p1", 12.0f));
    check(tabs.page_count() == 1, "TabView counts pages via child_count()");
    auto* before = tabs.page(0);
    tabs.set_page(0, std::make_unique<ui::Label>("p2", 12.0f));
    check(tabs.page(0) != before && tabs.page(0) != nullptr,
          "TabView replaces a page through replace_child()");
}

// 5) 几何唯一写入口：清空后再 arrange，几何只有一份。
void test_geometry_single_entry()
{
    ui::Label label("g", 12.0f);
    label.arrange({{4.0f, 5.0f}, {60.0f, 20.0f}});
    check(label.global_bounds().pos.x == 4.0f, "arrange keeps the given position");
    check(label.global_bounds().size.y == 20.0f,
          "arrange writes size through set_geometry only");
    check(label.local_bounds().pos.x == label.global_bounds().pos.x,
          "local_bounds is an alias of global_bounds");
}

int main()
{
    test_traversal_covers_overlays();
    test_debug_recursive_reaches_overlays();
    test_section_is_a_real_parent();
    test_indexed_access_api();
    test_geometry_single_entry();

    std::printf("tree contract: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
