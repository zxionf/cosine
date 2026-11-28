// UI-47：Widget 减法的契约测试。
//
// 三条不变量（对应 .design/ui-redesign.md K5 与 docs/todo-ui.md R1）：
//   1. 16 个「从未被任何子类覆写」的虚函数已降级为普通函数——派生类即使写同名函数，
//      经基类指针调用也不会被多态分派（这是「假扩展点」的反证）；
//   2. 叶子默认 measure/arrange 与旧样板等价（首选取 clamp、arrange 只写几何），
//      二者现在是 LayoutNode 的默认实现（不再有 LeafNode 中间基类）；
//   3. clips_children() 由 overflow() 派生（不再是可覆写的独立开关）。
#include "ui/core/button.hpp"
#include "ui/core/label.hpp"
#include "ui/core/progress.hpp"
#include "ui/core/slider.hpp"
#include "ui/core/spacer.hpp"
#include "ui/node/widget.hpp"
#include "ui/style/theme.hpp"

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

bool same_rect(ui::Rect a, ui::Rect b)
{
    constexpr float kEps = 1e-4f;
    return std::fabs(a.pos.x - b.pos.x) < kEps && std::fabs(a.pos.y - b.pos.y) < kEps
        && std::fabs(a.size.x - b.size.x) < kEps && std::fabs(a.size.y - b.size.y) < kEps;
}

// 探针：故意对已降级的函数写同名实现，验证"经基类指针调用"不生效。
class Probe : public ui::Widget
{
public:
    glm::vec2 measure(glm::vec2) override { return {10.0f, 10.0f}; }
    void arrange(ui::Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList&) const override {}

    // 这些在 UI-47 前是 virtual；现在必须是普通函数（写同名函数只遮蔽、不多态）。
    void set_pos(glm::vec2 pos) { shadowed_pos = pos; }
    bool clips_children() const { return true; }
    void debug() { shadowed_debug = true; }

    glm::vec2 shadowed_pos{0.0f, 0.0f};
    bool shadowed_debug = false;
};

// 探针：叶子，验证 LayoutNode 的默认 measure/arrange。
class LeafProbe : public ui::Widget
{
public:
    LeafProbe()
    {
        set_preferred_size({42.0f, 18.0f});
        set_min_size({10.0f, 5.0f});
        set_max_size({100.0f, 40.0f});
    }
    void append_to(graphics::DrawList&) const override {}
};
} // namespace

// 1) 已降级的虚函数不再参与多态分派。
void test_dead_virtuals_no_longer_dispatch()
{
    Probe probe;
    ui::Widget* base = &probe;

    base->set_pos({7.0f, 9.0f});
    check(probe.shadowed_pos.x == 0.0f && probe.shadowed_pos.y == 0.0f,
          "set_pos is no longer virtual (derived shadow is not dispatched)");
    check(probe.x() == 7.0f && probe.y() == 9.0f,
          "set_pos still updates geometry through the base implementation");

    base->set_size({30.0f, 20.0f});
    check(base->width() == 30.0f && base->height() == 20.0f, "set_size is no longer virtual");

    base->set_global_bounds({{1.0f, 2.0f}, {3.0f, 4.0f}});
    check(same_rect(base->global_bounds(), {{1.0f, 2.0f}, {3.0f, 4.0f}}),
          "set_global_bounds is a plain function");

    base->debug();
    check(!probe.shadowed_debug && probe.is_debug(), "debug() is a plain function");

    // clips_children 由 overflow() 派生：Probe 的同名遮蔽不生效。
    check(!base->clips_children(), "clips_children derives from overflow() (Visible -> false)");

    // padding/margin 访问器不再是虚函数：值来自基类字段。
    base->set_padding(6.0f);
    check(base->padding_left() == 6.0f && base->padding_bottom() == 6.0f,
          "padding_* accessors are plain functions");
    base->set_margin(4.0f);
    check(base->margin_left() == 4.0f && base->margin_bottom() == 4.0f,
          "margin_* accessors are plain functions");

    check(base->parent() == nullptr, "parent() is a plain function");
}

// 2) 叶子的默认实现与旧样板等价（现居 LayoutNode）。
void test_leaf_defaults()
{
    LeafProbe leaf;

    const glm::vec2 measured = leaf.measure({1000.0f, 1000.0f});
    check(measured.x == 42.0f && measured.y == 18.0f,
          "LayoutNode::measure returns preferred size (clamped)");

    // min/max clamp 生效：preferred 超出 max 时被截断。
    leaf.set_preferred_size({500.0f, 500.0f});
    const glm::vec2 clamped = leaf.measure({1000.0f, 1000.0f});
    check(clamped.x == 100.0f && clamped.y == 40.0f, "LayoutNode::measure clamps to max_size");

    leaf.arrange({{5.0f, 6.0f}, {70.0f, 25.0f}});
    check(same_rect(leaf.local_bounds(), {{5.0f, 6.0f}, {70.0f, 25.0f}})
              && same_rect(leaf.global_bounds(), leaf.local_bounds()),
          "LayoutNode::arrange writes geometry only (local == global)");
    check(leaf.layout_dirty(), "LayoutNode::arrange does not clear the dirty flag by itself");
}

// 3) 叶子控件（直接继承 Widget）的契约仍是「arrange 只写几何」。
void test_leaf_controls_keep_contract()
{
    ui::Button button("b");
    ui::Label label("l", 16.0f);
    ui::Progress progress;
    ui::Slider slider;

    for (ui::Widget* w : {static_cast<ui::Widget*>(&button), static_cast<ui::Widget*>(&label),
                          static_cast<ui::Widget*>(&progress), static_cast<ui::Widget*>(&slider)}) {
        w->arrange({{3.0f, 4.0f}, {55.0f, 22.0f}});
        check(same_rect(w->global_bounds(), {{3.0f, 4.0f}, {55.0f, 22.0f}}),
              "migrated leaf accepts the given rect");
        check(w->overflow() == ui::Widget::Overflow::Visible && !w->clips_children(),
              "migrated leaf keeps clips_children == false");
    }
}

// 4) 容纳子节点是 Node 的能力：叶子控件同样能 add（默认转发 adopt）。
void test_containment_lives_on_node()
{
    ui::Spacer spacer(glm::vec2{8.0f, 8.0f});
    spacer.add(std::make_unique<ui::Label>("child"));
    check(spacer.child_count() == 1, "any node can host children (Node::add defaults to adopt)");
    check(spacer.child_at(0) != nullptr && spacer.child_at(0)->parent() == &spacer,
          "Node::add wires the parent link through adopt()");
}

int main()
{
    test_dead_virtuals_no_longer_dispatch();
    test_leaf_defaults();
    test_leaf_controls_keep_contract();
    test_containment_lives_on_node();

    std::printf("node contract: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
