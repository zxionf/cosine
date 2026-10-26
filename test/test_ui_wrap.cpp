#include "ui/align.hpp"
#include "ui/color_rect.hpp"
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

void test_wrap_breaks_lines()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 100.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    for (int i = 0; i < 5; ++i) {
        wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    }

    // 内容宽 100：30 + 5 + 30 + 5 + 30 = 100 刚好放下 3 个，第 4 个换行。
    const glm::vec2 size = wrap.measure({100.0f, 1.0e9f});
    assert(nearly(size.y, 45.0f)); // 20 + 5 + 20

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    const auto& c = wrap.children();
    assert(nearly(c[0]->x(), 0.0f) && nearly(c[0]->y(), 0.0f));
    assert(nearly(c[1]->x(), 35.0f));
    assert(nearly(c[2]->x(), 70.0f));
    // 第二行
    assert(nearly(c[3]->x(), 0.0f) && nearly(c[3]->y(), 25.0f));
    assert(nearly(c[4]->x(), 35.0f) && nearly(c[4]->y(), 25.0f));
}

void test_wrap_unbounded_single_row()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 0.0f, 0.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    for (int i = 0; i < 5; ++i) {
        wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    }

    const glm::vec2 size = wrap.measure({1.0e9f, 1.0e9f});
    assert(nearly(size.x, 170.0f)); // 5*30 + 4*5
    assert(nearly(size.y, 20.0f));
}

void test_row_align_center()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 40.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    wrap.set_row_align(xel::ui::MainAlign::Center);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = wrap.children();
    // 行宽 65，剩余 35 → 居中偏移 17.5。
    assert(nearly(c[0]->x(), 17.5f));
    assert(nearly(c[1]->x(), 52.5f));
}

void test_row_align_end()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 40.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    wrap.set_row_align(xel::ui::MainAlign::End);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = wrap.children();
    assert(nearly(c[0]->x(), 35.0f));
    assert(nearly(c[1]->x(), 70.0f));
}

void test_row_align_space_between()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 40.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    wrap.set_row_align(xel::ui::MainAlign::SpaceBetween);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = wrap.children();
    // 剩余 35 全部进入间隙：5 + 35 = 40。
    assert(nearly(c[0]->x(), 0.0f));
    assert(nearly(c[1]->x(), 70.0f));
}

void test_cross_align_center()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 40.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 40, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 40.0f}});
    const auto& c = wrap.children();
    // 行高 40，20 高的子项垂直居中 → y = 10。
    assert(nearly(c[0]->y(), 10.0f));
    assert(nearly(c[1]->y(), 0.0f));
}

void test_oversized_child_own_row()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 100.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(5.0f);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 150, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    const auto& c = wrap.children();
    // 超宽子项独占一行，x 不为负；后续子项换到下一行。
    assert(nearly(c[0]->x(), 0.0f) && nearly(c[0]->y(), 0.0f));
    assert(nearly(c[1]->x(), 0.0f) && nearly(c[1]->y(), 25.0f));
}

void test_padding_and_margin()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 200.0f, 100.0f};
    wrap.set_padding(10.0f);
    wrap.set_spacing(0.0f);
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f});
    child->set_margin(5.0f);
    wrap.add(std::move(child));

    wrap.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    const auto& c = wrap.children();
    assert(nearly(c[0]->x(), 15.0f) && nearly(c[0]->y(), 15.0f));
}

void test_empty_wrap()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 100.0f};
    wrap.set_padding(0.0f);
    const glm::vec2 size = wrap.measure({100.0f, 100.0f});
    // 有限约束下块级容器占满可用宽；没有子项时高度为 0。
    assert(nearly(size.x, 100.0f) && nearly(size.y, 0.0f));
    wrap.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});

    // 零间距与负间距不应死循环或产生负坐标。
    wrap.set_spacing(-2.0f);
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 30, 20, glm::vec4{1.0f}));
    wrap.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    assert(wrap.children()[0]->x() >= 0.0f);
}
} // namespace

int main()
{
    test_wrap_breaks_lines();
    test_wrap_unbounded_single_row();
    test_row_align_center();
    test_row_align_end();
    test_row_align_space_between();
    test_cross_align_center();
    test_oversized_child_own_row();
    test_padding_and_margin();
    test_empty_wrap();
    std::cout << "test_ui_wrap: all passed\n";
    return 0;
}
