#include "ui/align.hpp"
#include "ui/color_rect.hpp"
#include "ui/grid.hpp"
#include "ui/hbox.hpp"
#include "ui/vbox.hpp"
#include "ui/wrap.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool nearly(float a, float b)
{
    return std::fabs(a - b) < 1e-3f;
}

void test_percent_width_in_vbox()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.5f, 0.0f});
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    // 宽 = 400 * 0.5 = 200；默认 top_center 居中。
    assert(nearly(c[0]->width(), 200.0f));
    assert(nearly(c[0]->x(), 100.0f));
}

void test_percent_beats_stretch()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    box.set_cross_align(xel::ui::Align::h_stretch());
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.25f, 0.0f});
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 声明了百分比就不再被 stretch 拉满。
    assert(nearly(box.children()[0]->width(), 100.0f));
}

void test_percent_ignored_when_unbounded()
{
    xel::ui::Vbox box{0.0f, 0.0f, 0.0f, 0.0f};
    box.set_padding(0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.5f, 0.0f});
    box.add(std::move(child));

    const glm::vec2 size = box.measure({1.0e9f, 1.0e9f});
    // 无界约束下百分比不生效，退回自然宽。
    assert(nearly(size.x, 50.0f));
}

void test_aspect_from_width()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto card = std::make_unique<xel::ui::ColorRect>(0, 0, 200, 50, glm::vec4{1.0f});
    card->set_aspect_ratio(2.0f);
    box.add(std::move(card));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    assert(nearly(c[0]->width(), 200.0f));
    assert(nearly(c[0]->height(), 100.0f)); // 高 = 宽 / 2
}

void test_aspect_from_height_percent()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto card = std::make_unique<xel::ui::ColorRect>(0, 0, 100, 50, glm::vec4{1.0f});
    card->set_size_percent({0.0f, 0.5f});
    card->set_aspect_ratio(2.0f);
    box.add(std::move(card));
    // Vbox 的高度百分比需要父级高度约束：measure 用 inner_h = 300。
    box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    // 高 = 300 * 0.5 = 150；宽 = 高 * 2 = 300。
    assert(nearly(c[0]->height(), 150.0f));
    assert(nearly(c[0]->width(), 300.0f));
}

void test_grid_cell_percent()
{
    xel::ui::Grid grid{0.0f, 0.0f, 200.0f, 100.0f};
    grid.set_padding(0.0f);
    grid.set_columns(2);
    grid.set_spacing(0.0f, 0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.5f, 0.0f});
    grid.add(std::move(child));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    const auto& c = grid.children();
    // 格宽 100，百分比 0.5 → 50。
    assert(nearly(c[0]->width(), 50.0f));
    // 未声明百分比的子项被 h_stretch 拉满格宽。
    assert(nearly(c[1]->width(), 100.0f));
}

void test_wrap_percent()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 200.0f, 100.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f});
    child->set_size_percent({0.5f, 0.0f});
    wrap.add(std::move(child));

    wrap.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    assert(nearly(wrap.children()[0]->width(), 100.0f));
}

void test_min_max_still_applies()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.5f, 0.0f});
    child->set_max_size({150.0f, 1.0e6f});
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    assert(nearly(box.children()[0]->width(), 150.0f));
}

void test_no_spec_unchanged()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f}));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 未声明时保持自然尺寸（与迁移前一致）。
    assert(nearly(box.children()[0]->width(), 50.0f));
    assert(nearly(box.children()[0]->height(), 20.0f));
}

void test_hbox_percent_height()
{
    xel::ui::Hbox row{0.0f, 0.0f, 400.0f, 200.0f};
    row.set_padding(0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_size_percent({0.0f, 0.5f});
    row.add(std::move(child));

    row.measure({400.0f, 200.0f});
    row.arrange({{0.0f, 0.0f}, {400.0f, 200.0f}});
    assert(nearly(row.children()[0]->height(), 100.0f));
}
} // namespace

int main()
{
    test_percent_width_in_vbox();
    test_percent_beats_stretch();
    test_percent_ignored_when_unbounded();
    test_aspect_from_width();
    test_aspect_from_height_percent();
    test_grid_cell_percent();
    test_wrap_percent();
    test_min_max_still_applies();
    test_no_spec_unchanged();
    test_hbox_percent_height();
    std::cout << "test_ui_percent_aspect: all passed\n";
    return 0;
}
