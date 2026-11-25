// UI-32 主轴分配与尺寸语义。
//
// 覆盖：
//   - pct 主轴预算（Vbox / Hbox / Section 内）
//   - stretch 后重新 clamp；stretch 与 size_percent / fill / fixed_size 的互斥
//   - aspect 与 clamp 一致求解（clamp 后回算另一轴，不破坏比例）
//   - fixed_size 在 stretch / flex 前短路
//   - 负 extra 的 flex shrink（下限 = min_size）
//   - 叶子构造期 preferred 自然尺寸（拉伸后可回缩）
//   - Hbox 的 flex 主轴规则与 Vbox 对偶
//   - 组合：pct+margin / stretch+max / shrink+min / 拉伸后回缩
#include "graphics/drawable.hpp"
#include "ui/containers/grid.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/containers/wrap.hpp"
#include "ui/core/button.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/image.hpp"
#include "ui/core/label.hpp"
#include "ui/layout_budget.hpp"
#include "ui/layout_metrics.hpp"
#include "ui/views/tree_view.hpp"

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

// 固定自然尺寸的最小控件（不走 strip 之外的逻辑）。
struct Counted : ui::Widget
{
    glm::vec2 natural{10.0f, 10.0f};

    Counted() { set_preferred_size(natural); }
    explicit Counted(glm::vec2 size) : natural(size) { set_preferred_size(natural); }

    glm::vec2 measure(glm::vec2) override { return clamp_size(preferred_size()); }
    void arrange(ui::Rect slot) override { global_ = slot; set_geometry(slot); }
    void append_to(graphics::DrawList&) const override {}
};

// ------------------------------------------------------------------ 预算

void test_pct_budget_halves()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    auto a = std::make_unique<Counted>();
    auto b = std::make_unique<Counted>();
    a->set_size_percent({0.0f, 1.0f});
    b->set_size_percent({0.0f, 1.0f});
    Counted* ap = a.get();
    Counted* bp = b.get();
    box.add(std::move(a));
    box.add(std::move(b));

    const glm::vec2 m = box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(m.y, 300.0f) && near(ap->height(), 150.0f) && near(bp->height(), 150.0f),
          "pct budget: 2x pct_y=1 -> 150/150",
          std::to_string(ap->height()) + "/" + std::to_string(bp->height()));
}

void test_pct_budget_with_margins()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    auto a = std::make_unique<Counted>();
    auto b = std::make_unique<Counted>();
    a->set_size_percent({0.0f, 1.0f});
    b->set_size_percent({0.0f, 1.0f});
    a->set_margin(ui::Edges{0.0f, 10.0f, 0.0f, 10.0f});
    b->set_margin(ui::Edges{0.0f, 10.0f, 0.0f, 10.0f});
    Counted* ap = a.get();
    Counted* bp = b.get();
    box.add(std::move(a));
    box.add(std::move(b));

    const glm::vec2 m = box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 可用 300 - 4*10 margins = 260；每个 pct_y=1 → 130
    check(near(ap->height(), 130.0f) && near(bp->height(), 130.0f) && near(m.y, 300.0f),
          "pct budget with margins: 130/130, total 300",
          std::to_string(ap->height()) + "/" + std::to_string(bp->height()) + "/" +
              std::to_string(m.y));
}

void test_pct_budget_hbox_three()
{
    ui::Hbox row(0.0f, 0.0f, 400.0f, 100.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    std::unique_ptr<Counted> items[3];
    for (int i = 0; i < 3; ++i) {
        items[i] = std::make_unique<Counted>();
        items[i]->set_size_percent({0.5f, 0.0f});
        row.add(std::move(items[i]));
    }

    const glm::vec2 m = row.measure({400.0f, 100.0f});
    row.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});
    // 三个 pct_x=0.5 总和 1.5 > 1 → 按权重 1/3 压缩：每个 400 / 3 ≈ 133.33
    const auto& c = row.children();
    const float each = 400.0f / 3.0f;
    check(near(m.x, 400.0f) && near(c[0]->width(), each) && near(c[1]->width(), each)
              && near(c[2]->width(), each),
          "pct budget: 3x pct_x=0.5 -> each 133.33",
          std::to_string(c[0]->width()));
}

void test_pct_budget_mixed_flex()
{
    ui::Hbox row(0.0f, 0.0f, 400.0f, 100.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    auto a = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    a->set_size_percent({0.25f, 0.0f});
    auto b = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    b->set_flex_grow(1.0f);
    Counted* bp = b.get();
    row.add(std::move(a));
    row.add(std::move(b));

    row.measure({400.0f, 100.0f});
    row.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});
    // a = 400 * 0.25 = 100；剩余 300 全部给 flex。pct 不参与 flex 分配。
    check(near(row.children()[0]->width(), 100.0f) && near(bp->width(), 300.0f),
          "pct reserves budget, flex fills remainder",
          std::to_string(row.children()[0]->width()) + "/" + std::to_string(bp->width()));
}

// ---------------------------------------------------------------- stretch

void test_stretch_respects_max()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_cross_align(ui::Align::h_stretch());
    auto child = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    child->set_max_size({200.0f, 1000.0f});
    Counted* p = child.get();
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // stretch 后 clamp：400 → 200
    check(near(p->width(), 200.0f), "stretch re-clamps to max_size", std::to_string(p->width()));
}

void test_stretch_skips_pct_and_fill()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_cross_align(ui::Align::h_stretch());
    auto a = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    a->set_size_percent({0.25f, 0.0f});
    auto b = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    b->set_fill(true, false);
    Counted* ap = a.get();
    Counted* bp = b.get();
    box.add(std::move(a));
    box.add(std::move(b));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(ap->width(), 100.0f) && near(bp->width(), 400.0f),
          "stretch skips pct and fill (pct=0.25 -> 100, fill -> 400)",
          std::to_string(ap->width()) + "/" + std::to_string(bp->width()));
}

void test_stretch_skips_fixed_size()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_cross_align(ui::Align::h_stretch());

    // TreeView 声明 fixed_size？实际 Table/FileBrowser 才 fixed_size；这里用自定义控件。
    struct FixedBox : Counted
    {
        FixedBox() : Counted(glm::vec2{120.0f, 40.0f}) {}
        bool fixed_size() const override { return true; }
    };
    auto child = std::make_unique<FixedBox>();
    FixedBox* p = child.get();
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(p->width(), 120.0f), "stretch skips fixed_size", std::to_string(p->width()));
}

// ----------------------------------------------------------------- aspect

void test_aspect_survives_max_clamp()
{
    // 宽 = 400（pct 1.0），ratio=2 → 高应 = 200，max 高 50 → 高被截断后回算宽 = 100。
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    auto card = std::make_unique<Counted>(glm::vec2{50.0f, 50.0f});
    card->set_size_percent({1.0f, 0.0f});
    card->set_aspect_ratio(2.0f);
    card->set_max_size({1.0e6f, 50.0f});
    Counted* p = card.get();
    box.add(std::move(card));

    box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(p->width(), 100.0f) && near(p->height(), 50.0f),
          "aspect survives max clamp (400x200 clamp h=50 -> 100x50)",
          std::to_string(p->width()) + "x" + std::to_string(p->height()));
}

void test_aspect_survives_min_clamp()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    auto card = std::make_unique<Counted>(glm::vec2{50.0f, 50.0f});
    card->set_size_percent({0.125f, 0.0f}); // 宽 = 50
    card->set_aspect_ratio(2.0f);           // 高 = 25
    card->set_min_size({0.0f, 60.0f});      // 高被拉高 → 宽回算 = 120
    Counted* p = card.get();
    box.add(std::move(card));

    box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    check(near(p->width(), 120.0f) && near(p->height(), 60.0f),
          "aspect survives min clamp (50x25 min h=60 -> 120x60)",
          std::to_string(p->width()) + "x" + std::to_string(p->height()));
}

// ------------------------------------------------------------------ shrink

void test_flex_shrink_to_min()
{
    ui::Hbox row(0.0f, 0.0f, 200.0f, 100.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    auto a = std::make_unique<Counted>(glm::vec2{150.0f, 20.0f});
    a->set_flex_grow(1.0f);
    a->set_min_size({80.0f, 0.0f});
    auto b = std::make_unique<Counted>(glm::vec2{150.0f, 20.0f});
    b->set_flex_grow(1.0f);
    b->set_min_size({80.0f, 0.0f});
    Counted* ap = a.get();
    Counted* bp = b.get();
    row.add(std::move(a));
    row.add(std::move(b));

    row.measure({200.0f, 100.0f});
    row.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    // flex 子项 natural = min_size = 80，总 160 <= 200 → 不 shrink，反而 grow 20。
    check(near(ap->width(), 100.0f) && near(bp->width(), 100.0f),
          "flex with natural=min_size grows to fill", std::to_string(ap->width()));
}

void test_flex_shrink_negative_extra()
{
    ui::Hbox row(0.0f, 0.0f, 200.0f, 100.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    // 非 flex 子项吃掉 100；两个 flex 子项 min_size=40、natural=min_size。
    auto fixed = std::make_unique<Counted>(glm::vec2{100.0f, 20.0f});
    auto a = std::make_unique<Counted>(glm::vec2{80.0f, 20.0f});
    a->set_flex_grow(1.0f);
    a->set_min_size({40.0f, 0.0f});
    auto b = std::make_unique<Counted>(glm::vec2{80.0f, 20.0f});
    b->set_flex_grow(1.0f);
    b->set_min_size({40.0f, 0.0f});
    Counted* ap = a.get();
    Counted* bp = b.get();
    row.add(std::move(fixed));
    row.add(std::move(a));
    row.add(std::move(b));

    row.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    // natural = 100 + 40 + 40 = 180，可用 200 → grow 20，每个 +10 → 50。
    check(near(ap->width(), 50.0f) && near(bp->width(), 50.0f),
          "flex natural=min_size -> grow evenly", std::to_string(ap->width()));
}

void test_flex_shrink_below_natural()
{
    // natural > min_size 时才有 shrink 空间：让 flex 子项 measure 回 80、min_size 40。
    ui::Hbox row(0.0f, 0.0f, 100.0f, 100.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    struct FlexBox : Counted
    {
        FlexBox() : Counted(glm::vec2{80.0f, 20.0f}) {}
        glm::vec2 measure(glm::vec2) override { return {80.0f, 20.0f}; }
    };
    auto a = std::make_unique<FlexBox>();
    a->set_flex_grow(1.0f);
    a->set_min_size({40.0f, 0.0f});
    auto b = std::make_unique<FlexBox>();
    b->set_flex_grow(1.0f);
    b->set_min_size({40.0f, 0.0f});
    row.add(std::move(a));
    row.add(std::move(b));
    // measure 后走 arrange；flex 子项 natural 按 min_size 计价 → 40+40=80 ≤100 → grow 各 +10。
    // 为了验证 shrink，这里改为非 flex 占位把 extra 压成负值。
    auto spacer = std::make_unique<Counted>(glm::vec2{60.0f, 20.0f});
    row.add(std::move(spacer));
    row.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    // natural = 40 + 40 + 60 = 140 > 100 → shrink 40，按 flex 各减 20 → 20/20；但 min 40 → 40/40
    const auto& c = row.children();
    check(near(c[0]->width(), 40.0f) && near(c[1]->width(), 40.0f),
          "flex shrink clamps at min_size", std::to_string(c[0]->width()) + "/" +
              std::to_string(c[1]->width()));
}

// ------------------------------------------------------------ leaf natural

void test_leaf_natural_stretch_then_shrink()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_cross_align(ui::Align::h_stretch());
    auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 80.0f, 20.0f, glm::vec4{1.0f});
    ui::ColorRect* p = rect.get();
    box.add(std::move(rect));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 拉伸后变 400
    const float stretched = p->width();
    // 容器缩短后再 stretch，仍应可回缩
    box.arrange({{0.0f, 0.0f}, {200.0f, 300.0f}});
    const float shrunk = p->width();
    // 去掉 stretch：恢复自然宽 80（不再记忆放大值）
    box.set_cross_align(ui::Align::top_left());
    box.arrange({{0.0f, 0.0f}, {200.0f, 300.0f}});
    const float natural = p->width();
    check(near(stretched, 400.0f) && near(shrunk, 200.0f) && near(natural, 80.0f),
          "ColorRect: stretch -> shrink -> natural 80 (no memory of 400)",
          std::to_string(stretched) + "/" + std::to_string(shrunk) + "/" +
              std::to_string(natural));
}

void test_leaf_image_preferred()
{
    ui::Image image(160.0f, 90.0f);
    const glm::vec2 m = image.measure({300.0f, 300.0f});
    check(near(m.x, 160.0f) && near(m.y, 90.0f), "Image natural = construction size",
          std::to_string(m.x) + "x" + std::to_string(m.y));
}

void test_leaf_tree_view_preferred()
{
    ui::TreeView tree(280.0f, 180.0f);
    const glm::vec2 m = tree.measure({100.0f, 100.0f});
    check(near(m.x, 280.0f) && near(m.y, 180.0f), "TreeView keeps preferred size",
          std::to_string(m.x) + "x" + std::to_string(m.y));
}

// ---------------------------------------------------------------- duality

void test_hbox_vbox_duality()
{
    // 同一组子项：Vbox 垂直排、Hbox 水平排；几何镜像（交换 x/y 后一致）。
    auto make_vbox = []() {
        auto b = std::make_unique<ui::Vbox>(0.0f, 0.0f, 300.0f, 200.0f);
        b->set_padding(0.0f);
        b->set_spacing(6.0f);
        b->set_cross_align(ui::Align::top_left());
        b->add(std::make_unique<Counted>(glm::vec2{40.0f, 20.0f}));
        auto f = std::make_unique<Counted>(glm::vec2{40.0f, 20.0f});
        f->set_flex_grow(1.0f);
        b->add(std::move(f));
        b->add(std::make_unique<Counted>(glm::vec2{40.0f, 20.0f}));
        return b;
    };
    auto make_hbox = []() {
        auto b = std::make_unique<ui::Hbox>(0.0f, 0.0f, 200.0f, 300.0f);
        b->set_padding(0.0f);
        b->set_spacing(6.0f);
        b->set_cross_align(ui::Align::top_left());
        b->add(std::make_unique<Counted>(glm::vec2{20.0f, 40.0f}));
        auto f = std::make_unique<Counted>(glm::vec2{20.0f, 40.0f});
        f->set_flex_grow(1.0f);
        b->add(std::move(f));
        b->add(std::make_unique<Counted>(glm::vec2{20.0f, 40.0f}));
        return b;
    };
    auto v = make_vbox();
    auto h = make_hbox();
    const glm::vec2 vm = v->measure({300.0f, 200.0f});
    const glm::vec2 hm = h->measure({200.0f, 300.0f});
    v->arrange({{0.0f, 0.0f}, {300.0f, 200.0f}});
    h->arrange({{0.0f, 0.0f}, {200.0f, 300.0f}});

    // Vbox 主轴高 = 20 + flex + 20 + 2*6 = natural 52；flex 子项 = 200 - 52 + 20 = 168
    // Hbox 主轴宽对应 = 20 + flex + 20 + 2*6 = 52；flex = 200 - 52 + 20 = 168
    const auto& vc = v->children();
    const auto& hc = h->children();
    const bool mirror = near(vc[0]->y(), hc[0]->x()) && near(vc[1]->height(), hc[1]->width())
        && near(vc[2]->y(), hc[2]->x()) && near(vm.y, hm.x);
    check(mirror, "Hbox/Vbox duality: same items mirrored on main axis",
          std::to_string(vc[1]->height()) + " vs " + std::to_string(hc[1]->width()));
}

void test_hbox_flex_natural_rule()
{
    // Hbox measure 现在与 Vbox 一致：flex 子项主轴只算 min_size，内容不撑大自然尺寸。
    ui::Hbox row(0.0f, 0.0f, 0.0f, 0.0f);
    row.set_padding(0.0f);
    row.set_spacing(0.0f);
    struct BigFlex : Counted
    {
        BigFlex() : Counted(glm::vec2{500.0f, 20.0f}) {}
        glm::vec2 measure(glm::vec2) override { return {500.0f, 20.0f}; }
    };
    auto f = std::make_unique<BigFlex>();
    f->set_flex_grow(1.0f);
    f->set_min_size({10.0f, 0.0f});
    row.add(std::move(f));

    const glm::vec2 m = row.measure({100.0f, 100.0f});
    // flex 子项 natural = min_size = 10，不报 500
    check(near(m.x, 10.0f), "hbox flex natural = min_size (not content width)",
          std::to_string(m.x));

    // Vbox 对偶：高度同理。
    ui::Vbox col(0.0f, 0.0f, 0.0f, 0.0f);
    col.set_padding(0.0f);
    col.set_spacing(0.0f);
    auto g = std::make_unique<BigFlex>();
    g->set_flex_grow(1.0f);
    g->set_min_size({0.0f, 10.0f});
    col.add(std::move(g));
    const glm::vec2 cm = col.measure({100.0f, 100.0f});
    check(near(cm.y, 10.0f), "vbox flex natural = min_size (mirror)",
          std::to_string(cm.y));
}

// --------------------------------------------------------------- combined

void test_combined_pct_margin_stretch()
{
    ui::Vbox box(0.0f, 0.0f, 400.0f, 300.0f);
    box.set_padding(0.0f);
    box.set_spacing(0.0f);
    box.set_cross_align(ui::Align::h_stretch());
    auto child = std::make_unique<Counted>(glm::vec2{50.0f, 20.0f});
    child->set_size_percent({0.5f, 0.0f});
    child->set_margin(ui::Edges{10.0f, 0.0f, 10.0f, 0.0f});
    Counted* p = child.get();
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 可用 = 400 - 20 = 380；pct 0.5 → 190；不 stretch（pct 已声明）
    check(near(p->width(), 190.0f) && near(p->x(), 10.0f),
          "pct+margin: 380*0.5=190 at x=10",
          std::to_string(p->width()) + "@" + std::to_string(p->x()));
}

void test_grid_aspect_clamp()
{
    ui::Grid grid(0.0f, 0.0f, 200.0f, 100.0f);
    grid.set_padding(0.0f);
    grid.set_columns(1);
    grid.set_spacing(0.0f, 0.0f);
    grid.set_cell_align(ui::Align::h_stretch());
    auto cell = std::make_unique<Counted>(glm::vec2{50.0f, 50.0f});
    cell->set_aspect_ratio(2.0f);
    cell->set_max_size({80.0f, 1.0e6f});
    Counted* p = cell.get();
    grid.add(std::move(cell));

    grid.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    // 格宽 200 → aspect 高 100；宽被 max 80 截断 → 高回算 40
    check(near(p->width(), 80.0f) && near(p->height(), 40.0f),
          "grid aspect + max clamp -> 80x40",
          std::to_string(p->width()) + "x" + std::to_string(p->height()));
}

void test_wrap_fixed_size_ignores_stretch()
{
    ui::Wrap wrap(0.0f, 0.0f, 200.0f, 100.0f);
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 0.0f);
    wrap.set_cross_align(ui::Align::v_stretch());
    struct FixedBox : Counted
    {
        FixedBox() : Counted(glm::vec2{80.0f, 30.0f}) {}
        bool fixed_size() const override { return true; }
    };
    auto p = std::make_unique<FixedBox>();
    FixedBox* fp = p.get();
    wrap.add(std::move(p));

    wrap.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    check(near(fp->height(), 30.0f), "wrap stretch skips fixed_size",
          std::to_string(fp->height()));
}

void test_budget_resolver_unit()
{
    // 直接验证 resolve_main_axis 的独立契约（不依赖具体容器）。
    ui::MainAxisItem a;
    a.pct = 0.5f;
    a.margin_before = 10.0f;
    a.margin_after = 10.0f;
    ui::MainAxisItem b;
    b.flex = 1.0f;
    b.natural = 20.0f;
    b.min_size = 20.0f;
    const auto sizes = ui::resolve_main_axis(200.0f, 0.0f, {a, b});
    // a: (200-20)*0.5 = 90；b natural 20 + flex 剩余 = 200 - 90 - 20 - 20 = 70 → b=90
    check(near(sizes[0], 90.0f) && near(sizes[1], 90.0f),
          "resolve_main_axis: pct+margin + flex share available",
          std::to_string(sizes[0]) + "/" + std::to_string(sizes[1]));
}

void test_request_size_updates_natural()
{
    ui::ColorRect rect(0.0f, 0.0f, 40.0f, 20.0f, glm::vec4{1.0f});
    check(near(rect.measure({100.0f, 100.0f}).x, 40.0f), "ColorRect starts at 40 wide",
          std::to_string(rect.measure({100.0f, 100.0f}).x));

    // set_size 只写几何：自然尺寸不变（避免被 arrange 污染）。
    rect.set_size(300.0f, 20.0f);
    check(near(rect.measure({1.0e6f, 1.0e6f}).x, 40.0f),
          "set_size does not change natural size",
          std::to_string(rect.measure({1.0e6f, 1.0e6f}).x));

    // request_size 是"用户显式改尺寸"：自然尺寸跟着变（调试面板用）。
    rect.request_size({120.0f, 60.0f});
    const glm::vec2 m = rect.measure({1.0e6f, 1.0e6f});
    check(near(m.x, 120.0f) && near(m.y, 60.0f), "request_size updates natural size",
          std::to_string(m.x) + "x" + std::to_string(m.y));
}

// UI-36：Align::baseline_align() 按文字基线对齐（Hbox 交叉轴）。
void test_baseline_alignment()
{
    ui::Hbox row(0.0f, 0.0f, 400.0f, 80.0f);
    row.set_padding(0.0f);
    row.set_spacing(8.0f);
    row.set_cross_align(ui::Align::baseline_align());
    auto label = std::make_unique<ui::Label>("Label", 14.0f);
    auto button = std::make_unique<ui::Button>("Button");
    ui::Label* lp = label.get();
    ui::Button* bp = button.get();
    row.add(std::move(label));
    row.add(std::move(button));
    row.arrange({{0.0f, 0.0f}, {400.0f, 80.0f}});

    const float label_baseline = lp->y() + lp->baseline_offset();
    const float button_baseline = bp->y() + bp->baseline_offset();
    check(near(label_baseline, button_baseline),
          "baseline align: label and button share a baseline",
          std::to_string(label_baseline) + " vs " + std::to_string(button_baseline));
    check(bp->height() > lp->height(), "button is taller than the label (mixed heights)",
          std::to_string(lp->height()) + "/" + std::to_string(bp->height()));
}

// UI-36：LayoutSpec 快照 + 回写闭环。
void test_layout_spec_roundtrip()
{
    ui::Widget::LayoutSpec spec;
    spec.align = ui::Align::center();
    spec.flex_grow = 2.0f;
    spec.size_percent = {0.5f, 0.0f};
    spec.aspect_ratio = 1.5f;
    spec.fill_x = true;
    spec.min_size = {10.0f, 20.0f};
    spec.max_size = {200.0f, 300.0f};

    ui::ColorRect rect(0.0f, 0.0f, 40.0f, 20.0f, glm::vec4{1.0f});
    rect.set_layout_spec(spec);
    const auto back = rect.layout_spec();
    check(back.align.has_value() && near(back.align->x, 0.5f) && near(back.flex_grow, 2.0f)
              && near(back.size_percent.x, 0.5f) && near(back.aspect_ratio, 1.5f)
              && back.fill_x && near(back.min_size.y, 20.0f) && near(back.max_size.x, 200.0f),
          "LayoutSpec round-trips through the widget fields");
}
} // namespace

int main()
{
    test_pct_budget_halves();
    test_pct_budget_with_margins();
    test_pct_budget_hbox_three();
    test_pct_budget_mixed_flex();
    test_stretch_respects_max();
    test_stretch_skips_pct_and_fill();
    test_stretch_skips_fixed_size();
    test_aspect_survives_max_clamp();
    test_aspect_survives_min_clamp();
    test_flex_shrink_to_min();
    test_flex_shrink_negative_extra();
    test_flex_shrink_below_natural();
    test_leaf_natural_stretch_then_shrink();
    test_leaf_image_preferred();
    test_leaf_tree_view_preferred();
    test_hbox_vbox_duality();
    test_hbox_flex_natural_rule();
    test_combined_pct_margin_stretch();
    test_grid_aspect_clamp();
    test_wrap_fixed_size_ignores_stretch();
    test_budget_resolver_unit();
    test_request_size_updates_natural();
    test_baseline_alignment();
    test_layout_spec_roundtrip();

    std::printf("main axis: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
