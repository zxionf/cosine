#include "ui/scroll_state.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace xel;

namespace
{
bool near(float a, float b)
{
    return std::fabs(a - b) < 0.01f;
}

void test_clamps_to_range()
{
    ui::ScrollState s;
    s.set_range(100.0f, 400.0f); // max = 300
    assert(near(s.max_offset(), 300.0f));
    s.jump_to(500.0f);
    assert(near(s.offset(), 300.0f));
    s.jump_to(-50.0f);
    assert(near(s.offset(), 0.0f));
    std::printf("clamp ok\n");
}

void test_jump_syncs_target()
{
    ui::ScrollState s;
    s.set_range(100.0f, 400.0f);
    s.jump_to(120.0f);
    assert(near(s.offset(), 120.0f));
    assert(near(s.target(), 120.0f));
    std::printf("jump syncs target ok\n");
}

void test_smooth_reaches_target()
{
    ui::ScrollState s;
    s.set_range(100.0f, 400.0f);
    s.set_smoothing(0.1f);
    s.scroll_by(200.0f, false);
    assert(near(s.offset(), 0.0f)); // 还没更新
    assert(near(s.target(), 200.0f));

    // 多帧逼近
    for (int i = 0; i < 200; ++i) s.update(0.016f);
    assert(near(s.offset(), 200.0f));
    std::printf("smooth reaches target ok\n");
}

void test_immediate_scroll()
{
    ui::ScrollState s;
    s.set_range(100.0f, 400.0f);
    s.scroll_by(80.0f, true);
    assert(near(s.offset(), 80.0f));
    assert(near(s.target(), 80.0f));
    std::printf("immediate scroll ok\n");
}

void test_range_shrink_reclamps()
{
    ui::ScrollState s;
    s.set_range(100.0f, 400.0f);
    s.jump_to(300.0f);
    s.set_range(100.0f, 200.0f); // max 变成 100
    assert(near(s.offset(), 100.0f));
    assert(near(s.target(), 100.0f));
    std::printf("range shrink reclamps ok\n");
}

void test_not_scrollable_resets()
{
    ui::ScrollState s;
    s.set_range(400.0f, 200.0f);
    assert(!s.scrollable());
    s.jump_to(50.0f);
    assert(near(s.offset(), 0.0f));
    assert(!s.update(0.016f));
    std::printf("not scrollable ok\n");
}
} // namespace

int main()
{
    test_clamps_to_range();
    test_jump_syncs_target();
    test_smooth_reaches_target();
    test_immediate_scroll();
    test_range_shrink_reclamps();
    test_not_scrollable_resets();
    std::printf("scroll state tests passed\n");
    return 0;
}
