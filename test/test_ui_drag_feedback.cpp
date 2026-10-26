#include "graphics/drawable.hpp"
#include "ui/color_rect.hpp"
#include "ui/slider.hpp"
#include "ui/splitter.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
using namespace xel;

size_t vertex_count(const graphics::DrawList& list)
{
    size_t count = 0;
    for (const auto& batch : list.batches()) count += batch.vertices.size();
    return count;
}

size_t draw_count(const ui::Widget& widget)
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 2000.0f, 2000.0f});
    widget.append_to(list);
    return vertex_count(list);
}

void test_slider_drag_shows_feedback()
{
    ui::Slider slider(200.0f);
    slider.set_range(0.0f, 100.0f);
    slider.set_step(1.0f);
    slider.set_value(40.0f);
    slider.arrange({{0.0f, 0.0f}, {200.0f, 30.0f}});

    const size_t idle = draw_count(slider);

    slider.on_mouse_down({100.0f, 15.0f}, 0);
    const size_t dragging = draw_count(slider);
    // 拖动中多了手柄描边环（无 font atlas 时气泡跳过）。
    assert(dragging > idle);

    slider.on_mouse_up({100.0f, 15.0f}, 0);
    assert(draw_count(slider) == idle);
}

void test_slider_value_stays_clamped()
{
    ui::Slider slider(200.0f);
    slider.set_range(0.0f, 100.0f);
    slider.set_step(1.0f);
    slider.arrange({{0.0f, 0.0f}, {200.0f, 30.0f}});

    slider.on_mouse_down({0.0f, 15.0f}, 0);
    slider.on_mouse_move({-500.0f, 15.0f});
    assert(slider.value() == 0.0f);

    slider.on_mouse_move({5000.0f, 15.0f});
    assert(slider.value() == 100.0f);
    slider.on_mouse_up({5000.0f, 15.0f}, 0);
}

void test_slider_fractional_step_format()
{
    // step < 1 时气泡用两位小数；无 atlas 时不应崩溃。
    ui::Slider slider(200.0f);
    slider.set_range(0.0f, 1.0f);
    slider.set_step(0.01f);
    slider.set_value(0.37f);
    slider.arrange({{0.0f, 0.0f}, {200.0f, 30.0f}});
    slider.on_mouse_down({100.0f, 15.0f}, 0);
    assert(draw_count(slider) > 0);
    slider.on_mouse_up({100.0f, 15.0f}, 0);
}

void test_splitter_drag_feedback_and_limits()
{
    ui::Splitter splitter(620.0f, 140.0f);
    auto left = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f,
                                                glm::vec4{0.2f, 0.3f, 0.4f, 1.0f});
    left->set_min_size({180.0f, 140.0f});
    auto right = std::make_unique<ui::ColorRect>(0.0f, 0.0f, 300.0f, 140.0f,
                                                 glm::vec4{0.3f, 0.5f, 0.3f, 1.0f});
    right->set_min_size({180.0f, 140.0f});
    splitter.set_panes(std::move(left), std::move(right));
    splitter.arrange({{0.0f, 0.0f}, {620.0f, 140.0f}});

    const float usable = 620.0f - 6.0f;
    const float expected_lower = 180.0f / usable;
    const float expected_upper = 1.0f - 180.0f / usable;

    const size_t idle = draw_count(splitter);

    // ratio_limits 由子项 min_size 推导。
    const auto limits = splitter.ratio_limits();
    assert(std::fabs(limits.first - expected_lower) < 1e-3f);
    assert(std::fabs(limits.second - expected_upper) < 1e-3f);

    const float handle_x = (620.0f - 6.0f) * splitter.ratio();
    assert(splitter.on_mouse_down({handle_x + 3.0f, 70.0f}, 0));

    // 拖动中多出强调色描边环。
    assert(draw_count(splitter) > idle);

    // 拖到最左/最右都被 min_size 限制。
    splitter.on_mouse_move({0.0f, 70.0f});
    assert(std::fabs(splitter.ratio() - expected_lower) < 1e-3f);

    splitter.on_mouse_move({620.0f, 70.0f});
    assert(std::fabs(splitter.ratio() - expected_upper) < 1e-3f);

    splitter.on_mouse_up({620.0f, 70.0f}, 0);
    // 松开后手柄恢复为普通填充；子项顶点数固定，总数回到按下前。
    assert(draw_count(splitter) == idle);
}

void test_splitter_without_min_size_falls_back()
{
    ui::Splitter splitter(400.0f, 100.0f);
    splitter.set_panes(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 200.0f, 100.0f,
                                                       glm::vec4{0.2f, 0.2f, 0.2f, 1.0f}),
                       std::make_unique<ui::ColorRect>(0.0f, 0.0f, 200.0f, 100.0f,
                                                       glm::vec4{0.3f, 0.3f, 0.3f, 1.0f}));
    splitter.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});

    const auto limits = splitter.ratio_limits();
    assert(std::fabs(limits.first - 0.1f) < 1e-4f);
    assert(std::fabs(limits.second - 0.9f) < 1e-4f);

    splitter.set_ratio(-5.0f);
    assert(std::fabs(splitter.ratio() - 0.1f) < 1e-4f);
    splitter.set_ratio(5.0f);
    assert(std::fabs(splitter.ratio() - 0.9f) < 1e-4f);
}

} // namespace

int main()
{
    test_slider_drag_shows_feedback();
    test_slider_value_stays_clamped();
    test_slider_fractional_step_format();
    test_splitter_drag_feedback_and_limits();
    test_splitter_without_min_size_falls_back();
    std::cout << "ui drag feedback tests passed\n";
    return 0;
}
