#include "ui/color_rect.hpp"
#include "ui/grid.hpp"
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

void test_absolute_out_of_flow_in_vbox()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    box.set_spacing(10.0f);
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 20, glm::vec4{1.0f}));

    auto badge = std::make_unique<xel::ui::ColorRect>(200, 100, 30, 10, glm::vec4{1.0f});
    badge->set_absolute(true);
    box.add(std::move(badge));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 30, glm::vec4{1.0f}));

    // 绝对定位子项不占高度：20 + 10 + 30 = 60（不含 badge）。
    const glm::vec2 size = box.measure({400.0f, 300.0f});
    assert(nearly(size.y, 60.0f));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    assert(nearly(c[1]->x(), 200.0f) && nearly(c[1]->y(), 100.0f));
    // 第三个流式子项紧跟第一个（中间没有 badge 的间距）。
    assert(nearly(c[2]->y(), 30.0f));
}

void test_absolute_percent_width()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto badge = std::make_unique<xel::ui::ColorRect>(10, 10, 30, 10, glm::vec4{1.0f});
    badge->set_absolute(true);
    badge->set_size_percent({0.5f, 0.0f});
    box.add(std::move(badge));

    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    assert(nearly(c[0]->width(), 200.0f));
    assert(nearly(c[0]->x(), 10.0f) && nearly(c[0]->y(), 10.0f));
}

void test_absolute_in_grid()
{
    xel::ui::Grid grid{0.0f, 0.0f, 200.0f, 100.0f};
    grid.set_padding(0.0f);
    grid.set_columns(2);
    grid.set_spacing(0.0f, 0.0f);
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    auto badge = std::make_unique<xel::ui::ColorRect>(5, 5, 10, 10, glm::vec4{1.0f});
    badge->set_absolute(true);
    grid.add(std::move(badge));
    grid.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 20, glm::vec4{1.0f}));

    grid.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    const auto& c = grid.children();
    // 流式项占第一行两格；badge 不占格，按自身坐标。
    assert(nearly(c[0]->x(), 0.0f) && nearly(c[0]->y(), 0.0f));
    assert(nearly(c[2]->x(), 100.0f) && nearly(c[2]->y(), 0.0f));
    assert(nearly(c[1]->x(), 5.0f) && nearly(c[1]->y(), 5.0f));
}

void test_absolute_in_wrap()
{
    xel::ui::Wrap wrap{0.0f, 0.0f, 100.0f, 100.0f};
    wrap.set_padding(0.0f);
    wrap.set_spacing(0.0f, 0.0f);

    auto badge = std::make_unique<xel::ui::ColorRect>(10, 10, 10, 10, glm::vec4{1.0f});
    badge->set_absolute(true);
    wrap.add(std::move(badge));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 60, 20, glm::vec4{1.0f}));
    wrap.add(std::make_unique<xel::ui::ColorRect>(0, 0, 60, 20, glm::vec4{1.0f}));

    wrap.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    const auto& c = wrap.children();
    // 两个 60 宽子项放不进一行 → 两行。
    assert(nearly(c[1]->y(), 0.0f));
    assert(nearly(c[2]->y(), 20.0f));
    // badge 不参与折行，按自身坐标放置。
    assert(nearly(c[0]->x(), 10.0f) && nearly(c[0]->y(), 10.0f));
}

void test_absolute_no_drift_across_layouts()
{
    xel::ui::Vbox box{0.0f, 0.0f, 200.0f, 100.0f};
    box.set_padding(0.0f);
    box.set_spacing(0.0f);

    auto badge = std::make_unique<xel::ui::ColorRect>(0, 0, 20, 10, glm::vec4{1.0f});
    badge->set_absolute_pos({30.0f, 40.0f});
    box.add(std::move(badge));

    // 多帧布局后位置不应漂移（arrange 会改写 local_.pos，absolute_pos 必须保持不变）。
    for (int i = 0; i < 4; ++i) {
        box.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    }
    const auto& c = box.children();
    assert(nearly(c[0]->x(), 30.0f));
    assert(nearly(c[0]->y(), 40.0f));
}

void test_paint_order_by_z()
{
    xel::ui::Vbox box{0.0f, 0.0f, 100.0f, 100.0f};
    auto a = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    auto b = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    auto c = std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f});
    a->set_z_index(0);
    b->set_z_index(5);
    c->set_z_index(-5);
    box.add(std::move(a));
    box.add(std::move(b));
    box.add(std::move(c));

    const auto order = box.paint_order();
    assert(order.size() == 3);
    assert(order[0] == box.children()[2].get()); // z = -5
    assert(order[1] == box.children()[0].get()); // z = 0
    assert(order[2] == box.children()[1].get()); // z = 5
}

void test_paint_order_stable_without_z()
{
    xel::ui::Vbox box{0.0f, 0.0f, 100.0f, 100.0f};
    for (int i = 0; i < 3; ++i) {
        box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 10, 10, glm::vec4{1.0f}));
    }
    const auto order = box.paint_order();
    assert(order.size() == 3);
    for (std::size_t i = 0; i < order.size(); ++i) {
        assert(order[i] == box.children()[i].get());
    }
}

void test_absolute_only_container()
{
    xel::ui::Vbox box{0.0f, 0.0f, 100.0f, 100.0f};
    box.set_padding(0.0f);
    auto badge = std::make_unique<xel::ui::ColorRect>(10, 20, 30, 10, glm::vec4{1.0f});
    badge->set_absolute(true);
    box.add(std::move(badge));

    const glm::vec2 size = box.measure({100.0f, 100.0f});
    assert(nearly(size.y, 0.0f)); // 流式高度为 0

    box.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    const auto& c = box.children();
    assert(nearly(c[0]->x(), 10.0f) && nearly(c[0]->y(), 20.0f));
}
} // namespace

int main()
{
    test_absolute_out_of_flow_in_vbox();
    test_absolute_percent_width();
    test_absolute_in_grid();
    test_absolute_in_wrap();
    test_absolute_no_drift_across_layouts();
    test_paint_order_by_z();
    test_paint_order_stable_without_z();
    test_absolute_only_container();
    std::cout << "test_ui_absolute_zindex: all passed\n";
    return 0;
}
