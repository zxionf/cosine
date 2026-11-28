// 布局 / 容器与视图 两个分区。
#include "sections/sections.hpp"

#include "ui/composites/tab_view.hpp"
#include "ui/containers/grid.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/containers/panel.hpp"
#include "ui/core/splitter.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/containers/wrap.hpp"
#include "ui/core/button.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/label.hpp"
#include "ui/core/spacer.hpp"
#include "ui/core/typography.hpp"

#include <memory>
#include <string>

namespace demo::sections
{
namespace
{
ui::ColorRect& cell(ui::Widget& parent, glm::vec4 color, float w = 0.0f, float h = 0.0f)
{
    auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, w, h, color);
    auto& ref = *rect;
    if (w <= 0.0f || h <= 0.0f) {
        // 没有显式尺寸时给一个默认首选大小，方便在流式布局里观察间距。
        ref.set_preferred_size({w > 0.0f ? w : 48.0f, h > 0.0f ? h : 24.0f});
    }
    parent.add(std::move(rect));
    return ref;
}

const glm::vec4 kBlue{0.24f, 0.56f, 0.91f, 1.0f};
const glm::vec4 kOrange{0.91f, 0.55f, 0.26f, 1.0f};
const glm::vec4 kGreen{0.36f, 0.78f, 0.51f, 1.0f};
const glm::vec4 kPurple{0.66f, 0.47f, 0.87f, 1.0f};
} // namespace

// ════════════════════════════════════════════════════════════════════
// 布局：Vbox / Hbox / Grid / Wrap / 对齐 / flex / 百分比 / 绝对定位
// ════════════════════════════════════════════════════════════════════
void build_layout(Demo& demo, ui::Vbox& column)
{
    auto& vertical = group(column, "Vbox：主轴对齐与间距");
    hint(vertical,
         "主轴对齐决定子项在剩余空间里的分布；交叉轴对齐决定每行内子项的水平位置。");
    auto& vertical_row = row(vertical);
    const ui::MainAlign aligns[] = {ui::MainAlign::Start, ui::MainAlign::Center,
                                    ui::MainAlign::End, ui::MainAlign::SpaceBetween};
    const char* labels[] = {"Start", "Center", "End", "SpaceBetween"};
    for (int i = 0; i < 4; ++i) {
        auto panel = std::make_unique<ui::Panel>(0.0f, 0.0f, 110.0f, 120.0f);
        panel->set_min_size({110.0f, 120.0f});
        panel->set_padding(6.0f);
        panel->set_spacing(4.0f);
        panel->set_main_align(aligns[i]);
        panel->set_cross_align(ui::Align::h_stretch());
        panel->add(std::make_unique<ui::Label>(labels[i], 0.0f));
        cell(*panel, kBlue, 0.0f, 12.0f);
        cell(*panel, kOrange, 0.0f, 12.0f);
        vertical_row.add(std::move(panel));
    }

    auto& flex = group(column, "flex_grow / 百分比 / aspect_ratio");
    hint(flex,
         "同一行里对比三种尺寸意图：固定首选宽、flex 均分剩余空间、百分比吃满可用宽。");
    auto& flex_row = row(flex);
    cell(flex_row, kBlue, 60.0f, 26.0f);

    auto grow = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 0.0f, 26.0f, kGreen);
    grow->set_flex_grow(1.0f);
    grow->set_min_size({40.0f, 26.0f});
    flex_row.add(std::move(grow));

    auto grow2 = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 0.0f, 26.0f, kOrange);
    grow2->set_flex_grow(2.0f);
    grow2->set_min_size({40.0f, 26.0f});
    flex_row.add(std::move(grow2));

    auto percent = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 0.0f, 26.0f, kPurple);
    percent->set_size_percent(0.25f, 0.0f);
    percent->set_min_size({40.0f, 26.0f});
    flex_row.add(std::move(percent));

    auto aspect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 0.0f, 0.0f, kBlue);
    aspect->set_preferred_size({120.0f, 0.0f});
    aspect->set_aspect_ratio(2.0f); // 宽:高 = 2:1
    flex_row.add(std::move(aspect));

    auto& grid = group(column, "Grid / Wrap");
    hint(grid, "Grid 按列宽分配（等宽或显式列宽）；Wrap 按行流式换行，适合标签云。");
    auto grid_box = std::make_unique<ui::Grid>(0.0f, 0.0f, 0.0f, 0.0f);
    grid_box->set_columns(4);
    grid_box->set_spacing(6.0f, 6.0f);
    grid_box->set_fill(true, false);
    for (int i = 0; i < 8; ++i) {
        cell(*grid_box, (i % 2 == 0) ? kBlue : kPurple, 0.0f, 22.0f);
    }
    grid.add(std::move(grid_box));

    auto wrap_box = std::make_unique<ui::Wrap>(0.0f, 0.0f, 0.0f, 0.0f);
    wrap_box->set_spacing(6.0f, 6.0f);
    wrap_box->set_fill(true, false);
    for (int i = 0; i < 14; ++i) {
        cell(*wrap_box, (i % 3 == 0) ? kGreen : (i % 3 == 1 ? kOrange : kBlue), 56.0f, 20.0f);
    }
    grid.add(std::move(wrap_box));

    auto& absolute = group(column, "绝对定位 Positioned");
    hint(absolute,
         "绝对定位子项不参与流式尺寸计算，按相对父内容区的坐标摆放；层叠顺序由 layer/z_index 决定。");
    auto stage = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 160.0f);
    stage->set_padding(0.0f);
    stage->set_min_size({0.0f, 160.0f});
    auto* stage_ptr = &*stage;
    auto base = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 0.0f, 0.0f, glm::vec4{0.16f, 0.18f, 0.21f, 1.0f});
    base->set_fill(true, true);
    stage_ptr->add(std::move(base));

    auto pinned = [](glm::vec2 pos, glm::vec2 size, glm::vec4 color) {
        auto rect = std::make_unique<ui::ColorRect>(0.0f, 0.0f, size.x, size.y, color);
        rect->set_absolute(true);
        rect->set_absolute_pos(pos);
        return rect;
    };
    stage_ptr->add(pinned({20.0f, 20.0f}, {90.0f, 40.0f}, kBlue));
    stage_ptr->add(pinned({140.0f, 60.0f}, {120.0f, 50.0f}, kOrange));
    auto on_top = pinned({90.0f, 40.0f}, {120.0f, 50.0f}, kGreen);
    on_top->set_z_index(2);
    stage_ptr->add(std::move(on_top));
    absolute.add(std::move(stage));

    demo.note("布局分区：改变窗口宽度，看百分比与 flex 如何重算");
}

// ════════════════════════════════════════════════════════════════════
// 容器与视图：TabView / ScrollPanel / Splitter
// ════════════════════════════════════════════════════════════════════
void build_containers(Demo& demo, ui::Vbox& column)
{
    auto& tabs = group(column, "TabView");
    hint(tabs, "页签可在上/下/左/右；页面切换与内容尺寸由 TabView 统一管理。");
    auto view = std::make_unique<ui::TabView>();
    view->set_fill(true, false);
    view->set_min_size({0.0f, 150.0f});
    // 页面按内容高度量（不铺满滚动内容区，否则单页面会把整页撑高）。
    view->set_page_fill(false);

    auto page_one = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    page_one->set_padding(8.0f);
    page_one->set_spacing(6.0f);
    page_one->set_cross_align(ui::Align::top_left());
    page_one->add(std::make_unique<ui::Label>("第一页：普通内容列"));
    page_one->add(std::make_unique<ui::Button>("页内按钮", [&demo] { demo.note("TabView 第一页按钮"); }));
    view->add_page("基础", std::move(page_one));

    auto page_two = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    page_two->set_padding(8.0f);
    page_two->set_spacing(6.0f);
    page_two->set_cross_align(ui::Align::top_left());
    auto& page_two_row = row(*page_two);
    cell(page_two_row, kBlue, 40.0f, 24.0f);
    cell(page_two_row, kGreen, 40.0f, 24.0f);
    cell(page_two_row, kOrange, 40.0f, 24.0f);
    page_two->add(std::make_unique<ui::Paragraph>("第二页：横排色块 + 说明文本，切换页签时各自状态保留。"));
    view->add_page("色块", std::move(page_two));

    auto empty_page = std::make_unique<ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    empty_page->set_padding(8.0f);
    empty_page->add(std::make_unique<ui::Paragraph>("这一页故意留空：演示页签名与最小内容高度。"));
    view->add_page("空页", std::move(empty_page));
    tabs.add(std::move(view));

    auto& nested = group(column, "嵌套分组（不可折叠）");
    hint(nested, "分组只是「标题 + 内容列」的卡片，没有收起/展开：页内内容始终可见，只用页签切换视图。");
    auto inner_card = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 0.0f);
    inner_card->set_padding(8.0f);
    inner_card->set_spacing(4.0f);
    inner_card->set_cross_align(ui::Align::top_left());
    inner_card->set_fill(true, false);
    inner_card->add(std::make_unique<ui::Label>("嵌套卡片 A：标题之外的缩进与间距由 Panel 提供"));
    inner_card->add(std::make_unique<ui::Label>("第二行内容"));
    auto inner_card2 = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 0.0f);
    inner_card2->set_padding(8.0f);
    inner_card2->set_cross_align(ui::Align::top_left());
    inner_card2->set_fill(true, false);
    inner_card2->add(std::make_unique<ui::Paragraph>("嵌套卡片 B：段落会按卡片宽度重新换行。"));
    nested.add(std::move(inner_card));
    nested.add(std::move(inner_card2));

    auto& split = group(column, "Splitter：可拖动分栏");
    hint(split, "拖动中间把手改比例（或设置 fixed_first 固定一侧像素）。");
    auto splitter = std::make_unique<ui::Splitter>(0.0f, 0.0f, ui::Splitter::Orientation::Horizontal);
    splitter->set_fill(true, false);
    splitter->set_min_size({0.0f, 130.0f});
    splitter->set_ratio(0.4f);

    auto left = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 130.0f);
    left->set_padding(8.0f);
    left->set_cross_align(ui::Align::top_left());
    left->add(std::make_unique<ui::Label>("左侧：Panel + Label"));
    auto right = std::make_unique<ui::Panel>(0.0f, 0.0f, 0.0f, 130.0f);
    right->set_padding(8.0f);
    right->set_cross_align(ui::Align::top_left());
    right->add(std::make_unique<ui::Paragraph>("右侧：段落会按分栏后的宽度重新换行。"));
    splitter->set_panes(std::move(left), std::move(right));
    split.add(std::move(splitter));

    auto& scrolling = group(column, "ScrollPanel：内容超出时滚动");
    hint(scrolling, "内容区独立滚动，滚轮只在指针位于面板内时生效。");
    auto panel = std::make_unique<ui::ScrollPanel>(0.0f, 0.0f, 0.0f, 140.0f);
    panel->set_min_size({0.0f, 140.0f});
    panel->set_padding(8.0f);
    panel->content()->set_spacing(4.0f);
    panel->content()->set_cross_align(ui::Align::top_left());
    for (int i = 0; i < 14; ++i) {
        panel->content()->add(std::make_unique<ui::Label>(
            "行 " + std::to_string(i + 1) + " —— 上下滚动查看，切走再切回位置保留"));
    }
    scrolling.add(std::move(panel));
}
} // namespace demo::sections
