#include "ui/core/color_rect.hpp"
#include "ui/containers/scroll_view.hpp"
#include "ui/views/table.hpp"
#include "ui/containers/vbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
bool nearly(float a, float b)
{
    return std::fabs(a - b) < 1e-2f;
}

// 内容高 400、视口 100 的 ScrollView。
std::unique_ptr<xel::ui::ScrollView> make_scroll()
{
    auto scroll = std::make_unique<xel::ui::ScrollView>();
    auto content = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 0.0f, 0.0f);
    content->set_padding(0.0f);
    content->set_spacing(0.0f);
    for (int i = 0; i < 20; ++i) {
        content->add(std::make_unique<xel::ui::ColorRect>(0, 0, 100, 20, glm::vec4{1.0f}));
    }
    scroll->set_content(std::move(content));
    scroll->arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    return scroll;
}

void test_smooth_defers_offset()
{
    auto scroll = make_scroll();
    scroll->set_scroll_mode(xel::ui::ScrollMode::Smooth);
    scroll->set_scroll_speed(40.0f);
    scroll->set_scroll_smoothing(0.1f);

    assert(nearly(scroll->scroll_offset(), 0.0f));
    assert(scroll->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f}));

    // 滚轮只改目标，实际位置还没动。
    assert(nearly(scroll->scroll_offset(), 0.0f));
    assert(nearly(scroll->scroll_target().y, 40.0f));

    scroll->update(0.016f);
    assert(scroll->scroll_offset() > 0.0f);
    assert(scroll->scroll_offset() < 40.0f);
}

void test_smooth_converges_and_snaps()
{
    auto scroll = make_scroll();
    scroll->set_scroll_smoothing(0.1f);
    scroll->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f});

    for (int i = 0; i < 120; ++i) scroll->update(0.016f);
    assert(nearly(scroll->scroll_offset(), 40.0f));
}

void test_line_mode_immediate()
{
    auto scroll = make_scroll();
    scroll->set_scroll_mode(xel::ui::ScrollMode::Line);
    scroll->set_scroll_line_step(24.0f);

    scroll->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f});
    assert(nearly(scroll->scroll_offset(), 24.0f));
    assert(nearly(scroll->scroll_target().y, 24.0f));
}

void test_clamp_target()
{
    auto scroll = make_scroll();

    // 往上滚出范围：目标被 clamp 到 0。
    scroll->on_scroll({50.0f, 50.0f}, {0.0f, 5.0f});
    assert(nearly(scroll->scroll_target().y, 0.0f));

    // 大量向下滚：目标不超过 max_scroll。
    for (int i = 0; i < 50; ++i) scroll->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f});
    const float max = 400.0f - scroll->viewport_size().y;
    assert(nearly(scroll->scroll_target().y, max));
}

void test_set_scroll_offset_immediate()
{
    auto scroll = make_scroll();
    scroll->set_scroll_offset({0.0f, 120.0f});
    assert(nearly(scroll->scroll_offset(), 120.0f));
    assert(nearly(scroll->scroll_target().y, 120.0f));
}

void test_framerate_independent()
{
    auto a = make_scroll();
    auto b = make_scroll();
    a->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f});
    b->on_scroll({50.0f, 50.0f}, {0.0f, -1.0f});

    // 同样 0.32 秒，不同 dt 分割。
    for (int i = 0; i < 20; ++i) a->update(0.016f);
    for (int i = 0; i < 8; ++i) b->update(0.04f);

    assert(nearly(a->scroll_offset(), b->scroll_offset()));
}

void test_fast_mode_converges_sooner()
{
    auto smooth = make_scroll();
    auto fast = make_scroll();
    smooth->set_scroll_mode(xel::ui::ScrollMode::Smooth);
    fast->set_scroll_mode(xel::ui::ScrollMode::Fast);

    smooth->on_scroll({50.0f, 50.0f}, {0.0f, -3.0f});
    fast->on_scroll({50.0f, 50.0f}, {0.0f, -3.0f});
    const float target = fast->scroll_target().y;

    // 同样 0.096 秒：Fast 因为时间常数更小，应走得更远。
    for (int i = 0; i < 6; ++i) {
        smooth->update(0.016f);
        fast->update(0.016f);
    }
    assert(fast->scroll_offset() > smooth->scroll_offset());

    // 停止输入后 0.1s 再加速收尾，最终吸附到目标。
    for (int i = 0; i < 40; ++i) fast->update(0.016f);
    assert(nearly(fast->scroll_offset(), target));
}

void test_table_smooth_vs_line()
{
    xel::ui::Table table{300.0f, 120.0f};
    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 30; ++i) {
        rows.push_back({"row " + std::to_string(i), "value"});
    }
    table.set_rows(std::move(rows));
    table.arrange({{0.0f, 0.0f}, {300.0f, 120.0f}});

    table.set_scroll_mode(xel::ui::ScrollMode::Smooth);
    table.on_scroll({150.0f, 60.0f}, {0.0f, -1.0f});
    assert(nearly(table.scroll_offset(), 0.0f)); // 平滑：还没动
    assert(table.scroll_target() > 0.0f);
    table.update(0.016f);
    assert(table.scroll_offset() > 0.0f);

    table.set_scroll_value(0.0f);
    table.set_scroll_mode(xel::ui::ScrollMode::Line);
    table.on_scroll({150.0f, 60.0f}, {0.0f, -1.0f});
    assert(table.scroll_offset() > 0.0f); // 立即
}
} // namespace

int main()
{
    test_smooth_defers_offset();
    test_smooth_converges_and_snaps();
    test_line_mode_immediate();
    test_clamp_target();
    test_set_scroll_offset_immediate();
    test_framerate_independent();
    test_fast_mode_converges_sooner();
    test_table_smooth_vs_line();
    std::cout << "test_ui_smooth_scroll: all passed\n";
    return 0;
}
