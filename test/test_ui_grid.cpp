#include "ui/align.hpp"
#include "ui/color_rect.hpp"
#include "ui/grid.hpp"

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

void test_equal_columns()
{
    xel::ui::Grid grid{0.0f, 0.0f, 300.0f, 90.0f};
    grid.set_padding(0.0f);
    grid.set_columns(3);
    grid.set_spacing(0.0f, 0.0f);
    for (int i = 0; i < 6; ++i) {
        grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f}));
    }

    grid.arrange({{0.0f, 0.0f}, {300.0f, 90.0f}});
    const auto& c = grid.children();
    assert(c.size() == 6);

    // 3 列等分：每列 100；默认 h_stretch 让子项填满格宽。
    assert(nearly(c[0]->x(), 0.0f) && nearly(c[0]->y(), 0.0f));
    assert(nearly(c[0]->width(), 100.0f) && nearly(c[0]->height(), 20.0f));
    assert(nearly(c[1]->x(), 100.0f));
    assert(nearly(c[2]->x(), 200.0f));
    // 第二行
    assert(nearly(c[3]->x(), 0.0f) && nearly(c[3]->y(), 20.0f));
}

void test_fixed_and_flexible()
{
    xel::ui::Grid grid{0.0f, 0.0f, 300.0f, 40.0f};
    grid.set_padding(0.0f);
    grid.set_column_widths({80.0f, -1.0f});
    grid.set_spacing(8.0f, 0.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {300.0f, 40.0f}});
    const auto& c = grid.children();
    // 可用宽 = 300 - 8 = 292；第一列固定 80，第二列吃满 212。
    assert(nearly(c[0]->x(), 0.0f) && nearly(c[0]->width(), 80.0f));
    assert(nearly(c[1]->x(), 88.0f) && nearly(c[1]->width(), 212.0f));
}

void test_flexible_weights()
{
    xel::ui::Grid grid{0.0f, 0.0f, 300.0f, 40.0f};
    grid.set_padding(0.0f);
    grid.set_column_widths({-2.0f, -1.0f});
    grid.set_spacing(0.0f, 0.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {300.0f, 40.0f}});
    const auto& c = grid.children();
    assert(nearly(c[0]->width(), 200.0f));
    assert(nearly(c[1]->x(), 200.0f) && nearly(c[1]->width(), 100.0f));
}

void test_fixed_overflow_compresses()
{
    xel::ui::Grid grid{0.0f, 0.0f, 100.0f, 40.0f};
    grid.set_padding(0.0f);
    grid.set_column_widths({100.0f, 100.0f});
    grid.set_spacing(0.0f, 0.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = grid.children();
    // 固定列总和超过可用宽：等比压缩到各 50，不留负宽。
    assert(nearly(c[0]->width(), 50.0f));
    assert(nearly(c[1]->x(), 50.0f) && nearly(c[1]->width(), 50.0f));
}

void test_measure_unbounded_uses_natural_width()
{
    xel::ui::Grid grid{0.0f, 0.0f, 0.0f, 0.0f};
    grid.set_padding(0.0f);
    grid.set_columns(3);
    grid.set_spacing(0.0f, 0.0f);
    for (int i = 0; i < 6; ++i) {
        grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f}));
    }

    const glm::vec2 size = grid.measure({1.0e9f, 1.0e9f});
    // 弹性列取自然宽最大值 50 → 总宽 150；两行 → 总高 40。
    assert(nearly(size.x, 150.0f));
    assert(nearly(size.y, 40.0f));
}

void test_measure_row_height()
{
    xel::ui::Grid grid{0.0f, 0.0f, 200.0f, 0.0f};
    grid.set_padding(0.0f);
    grid.set_columns(2);
    grid.set_spacing(0.0f, 4.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 40, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 30, glm::vec4{1.0f}));

    const glm::vec2 size = grid.measure({200.0f, 1.0e9f});
    // 行高取行内最大：40 与 30，加上行间距 4。
    assert(nearly(size.y, 74.0f));
}

void test_cell_align_override()
{
    xel::ui::Grid grid{0.0f, 0.0f, 200.0f, 40.0f};
    grid.set_padding(0.0f);
    grid.set_columns(2);
    grid.set_spacing(0.0f, 0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f});
    child->set_align(xel::ui::Align::center());
    grid.add(std::move(child));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 40, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {200.0f, 40.0f}});
    const auto& c = grid.children();
    // 行高 = 行内最大内容高 40；50x20 的项在 100x40 的格内居中。
    assert(nearly(c[0]->width(), 50.0f) && nearly(c[0]->height(), 20.0f));
    assert(nearly(c[0]->x(), 25.0f) && nearly(c[0]->y(), 10.0f));
}

void test_margin()
{
    xel::ui::Grid grid{0.0f, 0.0f, 100.0f, 40.0f};
    grid.set_padding(0.0f);
    grid.set_columns(1);
    grid.set_spacing(0.0f, 0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    child->set_margin(10.0f);
    grid.add(std::move(child));

    grid.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = grid.children();
    assert(nearly(c[0]->x(), 10.0f) && nearly(c[0]->width(), 80.0f));
    assert(nearly(c[0]->y(), 10.0f));
}

void test_empty_and_invalid_spec()
{
    xel::ui::Grid grid{0.0f, 0.0f, 100.0f, 100.0f};
    grid.set_padding(0.0f);
    assert(grid.columns() == 1);

    grid.set_columns(0);
    assert(grid.columns() == 1);

    grid.set_columns(-3);
    assert(grid.columns() == 1);

    grid.set_column_widths({});
    assert(grid.columns() == 1);

    // 空网格的测量与排布不应崩溃。
    const glm::vec2 size = grid.measure({100.0f, 100.0f});
    assert(size.x >= 0.0f && size.y >= 0.0f);
    grid.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
}

void test_padding_and_spacing()
{
    xel::ui::Grid grid{0.0f, 0.0f, 320.0f, 100.0f};
    grid.set_padding(10.0f);
    grid.set_columns(2);
    grid.set_spacing(20.0f, 0.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {320.0f, 100.0f}});
    const auto& c = grid.children();
    // 内容宽 = 320 - 20 = 300，扣列间距 20 → 每列 140。
    assert(nearly(c[0]->x(), 10.0f) && nearly(c[0]->width(), 140.0f));
    assert(nearly(c[1]->x(), 170.0f) && nearly(c[1]->width(), 140.0f));
    assert(nearly(c[0]->y(), 10.0f));
}
} // namespace

int main()
{
    test_equal_columns();
    test_fixed_and_flexible();
    test_flexible_weights();
    test_fixed_overflow_compresses();
    test_measure_unbounded_uses_natural_width();
    test_measure_row_height();
    test_cell_align_override();
    test_margin();
    test_empty_and_invalid_spec();
    test_padding_and_spacing();
    std::cout << "test_ui_grid: all passed\n";
    return 0;
}
