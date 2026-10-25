#include "graphics/drawable.hpp"
#include "ui/color_rect.hpp"
#include "ui/scroll_view.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
std::unique_ptr<xel::ui::ScrollView> make_scroll(float content_w, float content_h,
                                                 xel::ui::ScrollView::Axis axis,
                                                 xel::ui::Rect slot)
{
    auto scroll = std::make_unique<xel::ui::ScrollView>();
    scroll->set_axis(axis);
    scroll->set_content(std::make_unique<xel::ui::ColorRect>(
        0.0f, 0.0f, content_w, content_h, glm::vec4{0.3f, 0.4f, 0.5f, 1.0f}));
    scroll->measure(slot.size);
    scroll->arrange(slot);
    return scroll;
}
} // namespace

int main()
{
    // ---------- 纵向滚动条 ----------
    {
        auto scroll = make_scroll(200.0f, 1000.0f, xel::ui::ScrollView::Axis::Vertical,
                                  {{0.0f, 0.0f}, {200.0f, 200.0f}});
        assert(scroll->scrollbar_visible(true));
        assert(!scroll->scrollbar_visible(false));

        const auto track = scroll->scrollbar_track_rect(true);
        const auto thumb = scroll->scrollbar_thumb_rect(true);
        assert(track.size.x > 0.0f);
        assert(thumb.size.y >= 20.0f);
        assert(thumb.size.y <= track.size.y);
        assert(thumb.pos.y >= track.pos.y - 0.01f);
        assert(thumb.pos.y + thumb.size.y <= track.pos.y + track.size.y + 0.01f);

        // 拖动滑块：向下拖到底，offset 到达最大值。
        const glm::vec2 start{thumb.pos.x + thumb.size.x * 0.5f, thumb.pos.y + thumb.size.y * 0.5f};
        assert(scroll->on_mouse_down(start, 0));
        assert(scroll->scroll_offset() == 0.0f);
        assert(scroll->on_mouse_move({start.x, start.y + 500.0f}));
        assert(scroll->scroll_offset() > 0.0f);
        // 纵向滚动条占用宽度，viewport 高度仍是 200 → 最大滚动 800。
        assert(std::fabs(scroll->scroll_offset() - 800.0f) < 0.5f);
        assert(scroll->on_mouse_up({start.x, start.y + 500.0f}, 0));

        // 抬起后移动不再改变 offset。
        scroll->set_scroll_offset({0.0f, 0.0f});
        assert(!scroll->on_mouse_move({start.x, start.y + 100.0f}));
        assert(scroll->scroll_offset() == 0.0f);
    }

    // ---------- 轨道翻页 ----------
    {
        auto scroll = make_scroll(200.0f, 1000.0f, xel::ui::ScrollView::Axis::Vertical,
                                  {{0.0f, 0.0f}, {200.0f, 200.0f}});
        const auto track = scroll->scrollbar_track_rect(true);
        const auto thumb = scroll->scrollbar_thumb_rect(true);
        // 点击滑块下方 → 向下翻页。
        assert(scroll->on_mouse_down({track.pos.x + 1.0f, thumb.pos.y + thumb.size.y + 10.0f}, 0));
        assert(scroll->scroll_offset() > 0.0f);
        scroll->set_scroll_offset({0.0f, 1000.0f});
        // 点击滑块上方 → 向上翻页。
        assert(scroll->on_mouse_down({track.pos.x + 1.0f, track.pos.y + 1.0f}, 0));
        assert(scroll->scroll_offset() < 800.0f);
    }

    // ---------- 双轴滚动条 ----------
    {
        auto scroll = make_scroll(1000.0f, 1000.0f, xel::ui::ScrollView::Axis::Both,
                                  {{0.0f, 0.0f}, {200.0f, 200.0f}});
        assert(scroll->scrollbar_visible(true));
        assert(scroll->scrollbar_visible(false));

        const auto h_thumb = scroll->scrollbar_thumb_rect(false);
        const glm::vec2 start{h_thumb.pos.x + h_thumb.size.x * 0.5f, h_thumb.pos.y + h_thumb.size.y * 0.5f};
        assert(scroll->on_mouse_down(start, 0));
        assert(scroll->on_mouse_move({start.x + 400.0f, start.y}));
        // 双轴时宽高都扣除滚动条占用（1000 - (200 - 10)）。
        assert(std::fabs(scroll->scroll_offset_x() - 810.0f) < 1.5f);
        assert(scroll->scroll_offset() == 0.0f);
    }

    // ---------- 内容未溢出时不显示滚动条 ----------
    {
        auto scroll = make_scroll(100.0f, 100.0f, xel::ui::ScrollView::Axis::Vertical,
                                  {{0.0f, 0.0f}, {200.0f, 200.0f}});
        assert(!scroll->scrollbar_visible(true));
        assert(!scroll->scrollbar_visible(false));
        assert(!scroll->on_mouse_down({190.0f, 100.0f}, 0));
    }

    // ---------- 绘制不崩溃且产生批次 ----------
    {
        auto scroll = make_scroll(200.0f, 1000.0f, xel::ui::ScrollView::Axis::Vertical,
                                  {{0.0f, 0.0f}, {200.0f, 200.0f}});
        xel::graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 200.0f, 200.0f});
        scroll->append_to(list);
        assert(!list.batches().empty());
    }

    std::cout << "ui scrollbar tests passed\n";
    return 0;
}
