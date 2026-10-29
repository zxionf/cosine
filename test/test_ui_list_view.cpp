#include "ui/list_view.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace xel;

namespace
{
class TestListView : public ui::ListView
{
public:
    explicit TestListView(std::size_t count) : count_(count) {}
    std::size_t item_count() const override { return count_; }
    void draw_row(graphics::DrawList&, std::size_t, ui::Rect, bool) const override {}
    std::size_t first_row() const { return first_visible_row(); }
    std::size_t visible_rows() const { return visible_row_count(); }

private:
    std::size_t count_;
};

void test_selection_clamps()
{
    TestListView list(5);
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});

    list.set_selected(99);
    assert(list.selected() == 4);
    list.set_selected(-3);
    assert(list.selected() == 0);
    std::printf("selection clamps ok\n");
}

void test_keyboard_moves()
{
    TestListView list(5);
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    list.set_selected(0);

    assert(list.on_key_down(event::KeyCode::Down, event::Modifier{}));
    assert(list.selected() == 1);
    assert(list.on_key_down(event::KeyCode::End, event::Modifier{}));
    assert(list.selected() == 4);
    assert(list.on_key_down(event::KeyCode::Home, event::Modifier{}));
    assert(list.selected() == 0);
    std::printf("keyboard moves ok\n");
}

void test_ensure_selected_visible_scrolls()
{
    TestListView list(50); // 50 * 20 = 1000 内容，视口 100
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});

    list.set_selected(40);
    // 应把 40 行滚进视口：offset 至少让 41*20 不超过 offset+100
    const float offset = list.scroll_offset();
    assert(offset > 0.0f);
    assert(41.0f * 20.0f <= offset + 100.0f + 0.01f);
    std::printf("ensure visible scrolls ok\n");
}

void test_scroll_clamped_to_content()
{
    TestListView list(50);
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});

    list.set_scroll_mode(ui::ScrollMode::Line);
    // 往下滚很多次，应停在 max = 1000 - 100 = 900
    for (int i = 0; i < 100; ++i) {
        list.on_scroll({10.0f, 10.0f}, {0.0f, -1.0f});
    }
    assert(std::fabs(list.scroll_offset() - 900.0f) < 0.01f);
    std::printf("scroll clamped ok\n");
}

void test_not_scrollable_rejects_scroll()
{
    TestListView list(2); // 40 < 100
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    assert(!list.on_scroll({10.0f, 10.0f}, {0.0f, -1.0f}));
    std::printf("not scrollable rejects ok\n");
}

void test_visible_range()
{
    TestListView list(50);
    list.set_row_height(20.0f);
    list.arrange({{0.0f, 0.0f}, {200.0f, 100.0f}});
    // 视口 100 / 行高 20 = 5 行，多算一行 → 6
    assert(list.visible_rows() == 6);
    assert(list.first_row() == 0);
    std::printf("visible range ok\n");
}
} // namespace

int main()
{
    test_selection_clamps();
    test_keyboard_moves();
    test_ensure_selected_visible_scrolls();
    test_scroll_clamped_to_content();
    test_not_scrollable_rejects_scroll();
    test_visible_range();
    std::printf("list view tests passed\n");
    return 0;
}
