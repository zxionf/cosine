// 布局探针（ui #3 评审的行为固化）。
//
// 两类用例：
//   CONTRACT —— 期望行为，作为回归网（必须通过）。
//   DEBT     —— 当前实现的已知缺陷，显式断言"现在就是这样"，并在注释里标明由哪个任务翻转。
//               修好后把对应断言改成期望值（任务 step 里会点名本文件）。
#include "graphics/drawable.hpp"
#include "ui/composites/section.hpp"
#include "ui/composites/scroll_panel.hpp"
#include "ui/containers/grid.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/containers/wrap.hpp"
#include "ui/core/button.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/label.hpp"
#include "ui/core/splitter.hpp"
#include "ui/layout_metrics.hpp"
#include "ui/root.hpp"

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
    return std::fabs(a - b) < 0.01f;
}

// 计数用的最小控件：measure 返回固定自然尺寸，arrange 走常规写入。
struct Counted : ui::Widget
{
    int measures = 0;
    glm::vec2 natural{10.0f, 10.0f};

    glm::vec2 measure(glm::vec2) override
    {
        ++measures;
        return natural;
    }
    void arrange(ui::Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }
    void append_to(graphics::DrawList&) const override {}
};

Counted* add_counted(ui::Widget& parent, glm::vec2 natural = {10.0f, 10.0f})
{
    auto child = std::make_unique<Counted>();
    child->natural = natural;
    Counted* ptr = child.get();
    parent.add(std::move(child));
    return ptr;
}

// 计数 Label：文字测量的代理指标（Label 有自己的单槽缓存，仍会计入调用次数）。
struct CountedLabel : ui::Label
{
    int measures = 0;
    explicit CountedLabel(std::string text, float size) : ui::Label(std::move(text), size) {}
    glm::vec2 measure(glm::vec2 constraints) override
    {
        ++measures;
        return ui::Label::measure(constraints);
    }
};

// ---------------------------------------------------------------- CONTRACT

// 1) 单个百分比子项按容器可用宽计价。
void contract_pct_single_child()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    auto* child = add_counted(box);
    child->set_size_percent({0.5f, 0.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(child->width(), 200.0f), "pct_x=0.5 -> 200 in a 400 wide vbox",
          std::to_string(child->width()));
}

// 2) aspect_ratio 由宽推高。
void contract_aspect_ratio()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    auto* child = add_counted(box);
    child->set_size_percent({0.5f, 0.0f});
    child->set_aspect_ratio(2.0f);
    box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(child->width(), 200.0f) && near(child->height(), 100.0f),
          "pct_x=0.5 + aspect 2 -> 200x100",
          std::to_string(child->width()) + "x" + std::to_string(child->height()));
}

// 3) min/max 在百分比之后仍然生效。
void contract_min_max_over_pct()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    auto* child = add_counted(box);
    child->set_size_percent({0.5f, 0.0f});
    child->set_max_size({150.0f, 1.0e6f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(child->width(), 150.0f), "pct 0.5 with max 150 -> 150", std::to_string(child->width()));
}

// 4) Hbox：宽度 = 子项宽 + spacing + margin。
void contract_hbox_spacing_and_margins()
{
    ui::Hbox row(0.0f, 0.0f, 400.0f, 60.0f);
    row.set_padding(0.0f);
    row.set_spacing(7.0f);
    auto* a = add_counted(row, {40.0f, 20.0f});
    a->set_margin({5.0f, 0.0f, 3.0f, 0.0f});
    add_counted(row, {40.0f, 20.0f});
    const glm::vec2 m = row.measure({400.0f, 60.0f});
    check(near(m.x, 5.0f + 40.0f + 3.0f + 7.0f + 40.0f), "hbox width = margins + spacing + children",
          std::to_string(m.x));
}

// 5) Grid：固定列 + 弹性列解析。
void contract_grid_columns()
{
    ui::Grid grid(0.0f, 0.0f, 300.0f, 100.0f);
    grid.set_padding(0.0f);
    grid.set_spacing(0.0f, 0.0f);
    grid.set_column_widths({100.0f, -1.0f});
    auto* a = add_counted(grid, {10.0f, 10.0f});
    auto* b = add_counted(grid, {10.0f, 10.0f});
    grid.arrange({{0.0f, 0.0f}, {300.0f, 100.0f}});
    check(near(a->width(), 100.0f) && near(b->width(), 200.0f),
          "grid fixed 100 + flex 1 in 300 -> 100/200",
          std::to_string(a->width()) + "/" + std::to_string(b->width()));
}

// 6) Wrap：超宽折行（行数决定高度）。
void contract_wrap_rows()
{
    ui::Wrap wrap(0.0f, 0.0f, 200.0f, 100.0f);
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 5.0f);
    for (int i = 0; i < 5; ++i) add_counted(wrap, {60.0f, 20.0f});
    const glm::vec2 m = wrap.measure({200.0f, 100.0f});
    // 60*3 = 180 <= 200；每行 3 个 → 2 行 → 20 + 5 + 20 = 45
    check(near(m.y, 45.0f), "wrap 5x60 wide in 200 -> 2 rows, h=45", std::to_string(m.y));
}

// 7) 布局脏标记沿 parent 传播（改了子项尺寸，root 必须知道）。
void contract_dirty_propagates_to_root()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    ui::UIRoot* root_ptr = root.get();
    auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, 400.0f, 300.0f);
    ui::Vbox* box_ptr = box.get();
    root->add(std::move(box));
    root->layout();
    root->clear_layout_dirty();
    box_ptr->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 10.0f, 10.0f, glm::vec4{1.0f}));
    check(root_ptr->layout_dirty(), "adding a child marks root dirty");
}

// 8) 文字测量基线（UI-34 用它对比"每轮布局的文本测量次数"）。
void contract_text_measure_baseline()
{
    ui::Vbox box(0.0f, 0.0f, 300.0f, 200.0f);
    box.set_padding(0.0f);
    CountedLabel* labels[3] = {};
    for (int i = 0; i < 3; ++i) {
        auto label = std::make_unique<CountedLabel>("row " + std::to_string(i), 16.0f);
        labels[i] = label.get();
        box.add(std::move(label));
    }
    box.measure({300.0f, 200.0f});
    box.arrange({{0.0f, 0.0f}, {300.0f, 200.0f}});
    int total = 0;
    for (auto* label : labels) total += label->measures;
    // 基线（UI-30）：父 measure 一次 + 父 arrange 一次 → 每个 Label 2 次。
    check(total == 6, "text measure baseline: 2 per label per layout", std::to_string(total));
}

// 9) Section 内容按无界高度测量：pct_y 子项不再被算成 0 高（UI-30 修 -1 哨兵）。
void contract_section_content_unbounded_height()
{
    auto plain = std::make_unique<ui::Section>("A");
    plain->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 10.0f, 20.0f, glm::vec4{1.0f}));
    const float plain_h = plain->measure({200.0f, 100.0f}).y;

    auto pct = std::make_unique<ui::Section>("B");
    auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 10.0f, 20.0f, glm::vec4{1.0f});
    rect->set_size_percent({0.0f, 1.0f});
    pct->add(std::move(rect));
    const float pct_h = pct->measure({200.0f, 100.0f}).y;

    check(near(plain_h, pct_h), "section: pct_y child keeps natural height (unbounded)",
          std::to_string(plain_h) + " vs " + std::to_string(pct_h));
}

// 10) Wrap 在无界宽度下排成一行。
void contract_wrap_unbounded_single_row()
{
    ui::Wrap wrap(0.0f, 0.0f, 200.0f, 100.0f);
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 5.0f);
    for (int i = 0; i < 5; ++i) add_counted(wrap, {60.0f, 20.0f});
    const glm::vec2 m = wrap.measure({ui::kUnbounded, 100.0f});
    check(near(m.x, 300.0f) && near(m.y, 20.0f), "wrap unbounded -> single row of 300x20",
          std::to_string(m.x) + "x" + std::to_string(m.y));
}

// 11) ScrollPanel 在无界约束下不会把 1e8 泄漏到测量结果。
void contract_scroll_panel_no_unbounded_leak()
{
    auto panel = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    panel->set_padding(0.0f);
    panel->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 100.0f, 400.0f, glm::vec4{1.0f}));
    const glm::vec2 bounded = panel->measure({200.0f, 120.0f});
    const glm::vec2 unbounded = panel->measure({ui::kUnbounded, ui::kUnbounded});
    check(near(bounded.x, 100.0f) && near(unbounded.x, 100.0f) && bounded.y < ui::kUnbounded,
          "scroll panel: content width, no 1e8 leak",
          std::to_string(bounded.x) + "/" + std::to_string(unbounded.x));
}

// 12) 无界判定本身：1e8 / 1e9 / -1 都算无界。
void contract_is_bounded_semantics()
{
    check(ui::is_bounded(0.0f) && ui::is_bounded(1.0e7f) && !ui::is_bounded(ui::kUnbounded)
              && !ui::is_bounded(1.0e9f) && !ui::is_bounded(-1.0f),
          "is_bounded: [0,1e8) 有界；-1/1e9 无界");
}

// 13) arrange 写 local_：容器的 local_bounds() 跟随最近一次 slot（UI-31 修）。
void contract_container_local_bounds_follows_slot()
{
    ui::Vbox box(0.0f, 0.0f, 100.0f, 100.0f);
    box.set_padding(0.0f);
    add_counted(box);
    box.arrange({{50.0f, 60.0f}, {400.0f, 300.0f}});
    check(near(box.local_bounds().size.x, 400.0f) && near(box.local_bounds().size.y, 300.0f)
              && near(box.x(), 50.0f) && near(box.y(), 60.0f),
          "container local_bounds == last arranged slot",
          std::to_string(box.local_bounds().size.x) + "@" + std::to_string(box.x()));
}

// 14) layout() 用最近 slot 重排，不会回退到构造矩形（UI-31 修）。
void contract_layout_reuses_last_slot()
{
    ui::Vbox box(0.0f, 0.0f, 100.0f, 100.0f);
    box.set_padding(0.0f);
    box.set_cross_align(ui::Align::h_stretch());
    auto* child = add_counted(box);
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    box.layout();
    check(near(child->width(), 400.0f), "container.layout() keeps the arranged width",
          std::to_string(child->width()));
}

// 15) 布局过程不再把树弄脏（UI-31 修：LayoutPassScope）。
void contract_layout_does_not_leave_tree_dirty()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    ui::UIRoot* root_ptr = root.get();
    auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, 400.0f, 300.0f);
    box->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 10.0f, 10.0f, glm::vec4{1.0f}));
    root->add(std::move(box));
    root->clear_layout_dirty();
    root->layout();
    check(!root_ptr->layout_dirty(), "layout() leaves the tree clean");
}

// 16) margin 全轴参与测量：pct_y=1 + 上下 margin 不再溢出（UI-31）。
void contract_margin_on_main_axis()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    auto* child = add_counted(box);
    child->set_size_percent({0.0f, 1.0f});
    child->set_margin(ui::Edges{0.0f, 20.0f, 0.0f, 20.0f});
    const glm::vec2 m = box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 可用高 = 300 - 40 = 260；pct_y=1 → 260；总高 = 20 + 260 + 20 = 300
    check(near(m.y, 300.0f) && near(child->height(), 260.0f),
          "pct_y=1 with margins uses (container - margins)",
          std::to_string(m.y) + " / " + std::to_string(child->height()));
}

// 18) Wrap 不再返回 > constraints（fill_x）：子项比容器宽时也只报可用宽。
void contract_wrap_measure_within_constraints()
{
    ui::Wrap wrap(0.0f, 0.0f, 200.0f, 100.0f);
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 0.0f);
    add_counted(wrap, {300.0f, 20.0f});
    const glm::vec2 m = wrap.measure({200.0f, 100.0f});
    check(near(m.x, 200.0f), "wrap.measure == constraints.x (fill, never wider)",
          std::to_string(m.x));
}

// ------------------------------------------------------------------- DEBT
// 每条都断言"当前缺陷仍然存在"。对应任务修好后把断言改成期望值。

// D1 → UI-31 已修：见 contract_wrap_measure_within_constraints。

// D2 pct 不共享预算：两个 pct_y=1 各拿整高（ui-v3 P0-2 / 由 UI-32 修）。
void debt_pct_has_no_budget()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    auto* a = add_counted(box);
    auto* b = add_counted(box);
    a->set_size_percent({0.0f, 1.0f});
    b->set_size_percent({0.0f, 1.0f});
    const glm::vec2 m = box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(m.y > 300.0f && b->global_bounds().pos.y >= 300.0f,
          "DEBT(P0-2/T26): 2x pct_y=1 overflows the container",
          std::to_string(m.y));
}

// D3 → UI-31 已修：见 contract_splitter_measures_panes。

// D4 → UI-31 已修：见 contract_container_local_bounds_follows_slot。

// D5 隐藏子项仍占位（ui-v3 P1-8 / 由 UI-33 修）。
void debt_hidden_child_still_occupied()
{
    ui::Vbox box(0.0f, 0.0f, 200.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    add_counted(box, {10.0f, 20.0f});
    auto* b = add_counted(box, {10.0f, 20.0f});
    const float before = box.measure({200.0f, 300.0f}).y;
    b->set_visible(false);
    const float after = box.measure({200.0f, 300.0f}).y;
    check(near(before, after), "DEBT(P1-8/T27): hidden child still reserves space",
          std::to_string(before) + " -> " + std::to_string(after));
}

// D6 measure 缓存跨布局不命中（ui-v3 P1-4 / 由 UI-34 决策）。
void debt_measure_cache_not_reused()
{
    ui::Vbox box(0.0f, 0.0f, 300.0f, 200.0f);
    box.set_padding(0.0f);
    auto* child = add_counted(box);
    box.measure({300.0f, 200.0f});                    // 1
    box.arrange({{0.0f, 0.0f}, {300.0f, 200.0f}});    // 2
    box.measure({300.0f, 200.0f});                    // 期望命中 → 仍是 2
    check(child->measures == 3, "DEBT(P1-4/T28): measure cache never hits across layouts",
          std::to_string(child->measures));
}

// D7 主题切换后 measure 缓存仍是旧值，且单遍布局会留下"容器 30 高、内容 60 高"
//    （ui-v3 P1-4 / 由 UI-34 修）。
void debt_theme_change_leaves_stale_measure()
{
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    auto outer = std::make_unique<ui::Vbox>(0.0f, 0.0f, 400.0f, 300.0f);
    outer->set_padding(0.0f);
    outer->set_spacing(0.0f);
    auto button = std::make_unique<ui::Button>("x");
    ui::Vbox* outer_ptr = outer.get();
    ui::Button* button_ptr = button.get();
    outer->add(std::move(button));
    root->add(std::move(outer));
    root->layout();

    // 关键：两次 layout 之间有人 measure 过 → 缓存被填充，且没有版本键。
    const float before = outer_ptr->measure({400.0f, 300.0f}).y;

    ui::Theme theme = root->ui_theme();
    theme.control_height = 60.0f;
    root->set_theme(theme);
    const float stale = outer_ptr->measure({400.0f, 300.0f}).y;

    root->invalidate_measure();
    root->layout();
    check(near(before, 30.0f) && near(stale, 30.0f) && near(button_ptr->height(), 60.0f),
          "DEBT(P1-4/T28): cached measure survives a theme change (no revision key)",
          std::to_string(before) + " -> " + std::to_string(stale));
}

// D8 → UI-31 已修：见 contract_layout_does_not_leave_tree_dirty。

// 17) Splitter 安排前先测量 panes（UI-31；ratio/overflow 仍属 UI-33）。
void contract_splitter_measures_panes()
{
    ui::Splitter splitter(400.0f, 300.0f, ui::Splitter::Orientation::Horizontal);
    auto left = std::make_unique<Counted>();
    auto right = std::make_unique<Counted>();
    Counted* lp = left.get();
    Counted* rp = right.get();
    splitter.set_panes(std::move(left), std::move(right));
    splitter.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(lp->measures >= 1 && rp->measures >= 1, "splitter measures both panes",
          std::to_string(lp->measures) + "/" + std::to_string(rp->measures));
}
} // namespace

int main()
{
    contract_pct_single_child();
    contract_aspect_ratio();
    contract_min_max_over_pct();
    contract_hbox_spacing_and_margins();
    contract_grid_columns();
    contract_wrap_rows();
    contract_dirty_propagates_to_root();
    contract_text_measure_baseline();
    contract_section_content_unbounded_height();
    contract_wrap_unbounded_single_row();
    contract_scroll_panel_no_unbounded_leak();
    contract_is_bounded_semantics();
    contract_container_local_bounds_follows_slot();
    contract_layout_reuses_last_slot();
    contract_layout_does_not_leave_tree_dirty();
    contract_margin_on_main_axis();
    contract_splitter_measures_panes();
    contract_wrap_measure_within_constraints();

    debt_pct_has_no_budget();
    debt_hidden_child_still_occupied();
    debt_measure_cache_not_reused();
    debt_theme_change_leaves_stale_measure();

    std::printf("layout probe: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
