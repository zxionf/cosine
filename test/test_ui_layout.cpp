#include "ui/align.hpp"
#include "ui/color_rect.hpp"
#include "ui/hbox.hpp"
#include "ui/root.hpp"
#include "ui/scroll_view.hpp"
#include "ui/spacer.hpp"
#include "ui/vbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool nearly(float a, float b)
{
    return std::fabs(a - b) < 1e-4f;
}

void test_align_names()
{
    const auto h = xel::ui::Align::h_stretch();
    const auto v = xel::ui::Align::v_stretch();
    const auto tl = xel::ui::Align::top_left();

    assert(h.stretch_x && !h.stretch_y);
    assert(!v.stretch_x && v.stretch_y);
    assert(!tl.stretch_x && !tl.stretch_y);
}

void test_vbox_measure_and_padding()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 600.0f};
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));

    const glm::vec2 size = box.measure({400.0f, 600.0f});
    // 3 * 50 + 2 * spacing(4) + 2 * padding(4)
    assert(nearly(size.y, 166.0f));
    assert(nearly(size.x, 58.0f));

    box.set_cross_align(xel::ui::Align::top_left());
    box.arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    const auto& children = box.children();
    assert(children.size() == 3);
    assert(nearly(children[0]->x(), 4.0f));
    assert(nearly(children[0]->y(), 4.0f));
    assert(nearly(children[0]->width(), 50.0f));
    assert(nearly(children[1]->y(), 58.0f));
    assert(nearly(children[2]->y(), 112.0f));
}

void test_vbox_stretch()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 600.0f};
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    child->set_align(xel::ui::Align::h_stretch());
    box.add(std::move(child));

    box.arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    const auto& children = box.children();
    assert(nearly(children[0]->x(), 4.0f));
    assert(nearly(children[0]->width(), 392.0f));
}

void test_scroll_view()
{
    auto scroll = std::make_unique<xel::ui::ScrollView>();
    auto content = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 400.0f, 0.0f);
    for (int i = 0; i < 20; ++i) {
        content->add(std::make_unique<xel::ui::ColorRect>(0, 0, 400, 100, glm::vec4{1.0f}));
    }

    auto* content_ptr = content.get();
    scroll->set_content(std::move(content));

    assert(scroll->children().size() == 1);
    assert(content_ptr->parent() == scroll.get());

    scroll->measure({400.0f, 600.0f});
    assert(scroll->content_size().y > 600.0f);

    scroll->arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    // 本用例验证「滚动位移与边界」，用离散模式避免平滑过渡的延迟生效。
    scroll->set_scroll_mode(xel::ui::ScrollMode::Line);
    assert(nearly(scroll->scroll_offset(), 0.0f));

    assert(scroll->on_scroll({10.0f, 10.0f}, {0.0f, -1.0f}));
    assert(nearly(scroll->scroll_offset(), 40.0f));

    assert(scroll->on_scroll({10.0f, 10.0f}, {0.0f, 1.0f}));
    assert(nearly(scroll->scroll_offset(), 0.0f));

    // 超出 viewport 不消费
    assert(!scroll->on_scroll({1000.0f, 1000.0f}, {0.0f, -1.0f}));
}

void test_root_parent_and_dirty()
{
    xel::ui::UIRoot root{0.0f, 0.0f, 800.0f, 600.0f};
    auto box = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 200.0f, 100.0f);
    auto* box_ptr = box.get();

    root.add(std::move(box));
    assert(box_ptr->parent() == &root);
    assert(root.children().size() == 1);

    root.clear_layout_dirty();
    box_ptr->set_pos(10.0f, 20.0f);
    assert(root.layout_dirty());
}

void test_rect_intersect()
{
    const xel::ui::Rect a{{0.0f, 0.0f}, {100.0f, 100.0f}};
    const xel::ui::Rect b{{50.0f, 50.0f}, {100.0f, 100.0f}};
    const auto i = a.intersect(b);
    assert(nearly(i.pos.x, 50.0f));
    assert(nearly(i.pos.y, 50.0f));
    assert(nearly(i.size.x, 50.0f));
    assert(nearly(i.size.y, 50.0f));
}
void test_hbox_measure_and_spacing()
{
    xel::ui::Hbox box{0.0f, 0.0f, 400.0f, 100.0f};
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));

    const glm::vec2 size = box.measure({400.0f, 100.0f});
    // 3 * 50 + 2 * spacing(4) + 2 * padding(4)
    assert(nearly(size.x, 166.0f));
    assert(nearly(size.y, 58.0f));

    box.set_cross_align(xel::ui::Align::top_left());
    box.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});
    const auto& children = box.children();
    assert(children.size() == 3);
    assert(nearly(children[0]->x(), 4.0f));
    assert(nearly(children[0]->y(), 4.0f));
    assert(nearly(children[1]->x(), 58.0f));
    assert(nearly(children[2]->x(), 112.0f));
}

void test_hbox_space_between_and_stretch()
{
    xel::ui::Hbox box{0.0f, 0.0f, 400.0f, 100.0f};
    box.set_main_align(xel::ui::MainAlign::SpaceBetween);

    auto a = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    auto b = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    b->set_align(xel::ui::Align::v_stretch());
    box.add(std::move(a));
    box.add(std::move(b));

    box.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});
    const auto& children = box.children();
    assert(nearly(children[0]->x(), 4.0f));
    assert(nearly(children[1]->x(), 346.0f));
    assert(nearly(children[1]->y(), 4.0f));
    assert(nearly(children[1]->height(), 92.0f));
}

void test_spacer()
{
    xel::ui::Hbox box{0.0f, 0.0f, 400.0f, 100.0f};
    box.add(std::make_unique<xel::ui::Spacer>(20.0f, 10.0f));
    box.add(std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f}));

    const glm::vec2 size = box.measure({400.0f, 100.0f});
    assert(nearly(size.x, 20.0f + 50.0f + 4.0f + 8.0f));
    assert(nearly(size.y, 50.0f + 8.0f));
}
void test_padding_and_margin()
{
    xel::ui::Vbox box{0.0f, 0.0f, 200.0f, 300.0f};
    box.set_padding({10.0f, 20.0f, 30.0f, 40.0f});

    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    child->set_margin({5.0f, 6.0f, 7.0f, 8.0f});
    box.add(std::move(child));

    const glm::vec2 size = box.measure({200.0f, 300.0f});
    assert(nearly(size.x, 50.0f + 5.0f + 7.0f + 10.0f + 30.0f));
    assert(nearly(size.y, 50.0f + 6.0f + 8.0f + 20.0f + 40.0f));

    box.arrange({{0.0f, 0.0f}, {200.0f, 300.0f}});
    const auto& c = box.children()[0];
    assert(nearly(c->x(), 10.0f + 5.0f + ((200.0f - 40.0f - 5.0f - 7.0f) - 50.0f) * 0.5f));
    assert(nearly(c->y(), 20.0f + 6.0f));
    assert(nearly(c->width(), 50.0f));
    assert(nearly(c->height(), 50.0f));
}

void test_flex_grow()
{
    xel::ui::Hbox box{0.0f, 0.0f, 400.0f, 100.0f};
    auto a = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    auto b = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    b->set_flex_grow(1.0f);
    box.add(std::move(a));
    box.add(std::move(b));

    box.arrange({{0.0f, 0.0f}, {400.0f, 100.0f}});
    const auto& children = box.children();
    // inner 392, natural 104, extra 288 全给 flex 子项
    assert(nearly(children[0]->width(), 50.0f));
    assert(nearly(children[1]->width(), 338.0f));
    assert(nearly(children[1]->x() + children[1]->width(), 396.0f));
}

void test_min_max_size()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 600.0f};
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    child->set_min_size({80.0f, 40.0f});
    child->set_max_size({120.0f, 60.0f});
    box.add(std::move(child));

    const glm::vec2 size = box.measure({400.0f, 600.0f});
    assert(nearly(size.x, 80.0f + 8.0f));
    assert(nearly(size.y, 50.0f + 8.0f));

    box.arrange({{0.0f, 0.0f}, {400.0f, 600.0f}});
    assert(nearly(box.children()[0]->width(), 80.0f));
    assert(nearly(box.children()[0]->height(), 50.0f));
}

void test_measure_cache_invalidation()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 600.0f};
    auto child = std::make_unique<xel::ui::ColorRect>(0, 0, 50, 50, glm::vec4{1.0f});
    auto* child_ptr = child.get();
    box.add(std::move(child));

    const glm::vec2 first = box.measure({400.0f, 600.0f});
    const glm::vec2 second = box.measure({400.0f, 600.0f});
    assert(nearly(first.x, second.x) && nearly(first.y, second.y));

    child_ptr->set_size(100.0f, 100.0f);
    const glm::vec2 third = box.measure({400.0f, 600.0f});
    assert(nearly(third.y, first.y + 50.0f));
}

void test_scroll_view_both_axes()
{
    auto scroll = std::make_unique<xel::ui::ScrollView>();
    scroll->set_axis(xel::ui::ScrollView::Axis::Both);

    auto content = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 800.0f, 800.0f);
    content->add(std::make_unique<xel::ui::ColorRect>(0, 0, 800, 800, glm::vec4{1.0f}));
    scroll->set_content(std::move(content));

    scroll->measure({400.0f, 300.0f});
    scroll->arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    // 双向滚动的位移断言基于离散模式。
    scroll->set_scroll_mode(xel::ui::ScrollMode::Line);

    assert(scroll->content_size().x > 400.0f);
    assert(scroll->content_size().y > 300.0f);

    assert(scroll->on_scroll({10.0f, 10.0f}, {-1.0f, -1.0f}));
    assert(nearly(scroll->scroll_offset_x(), 40.0f));
    assert(nearly(scroll->scroll_offset(), 40.0f));
}

void test_draw_list_clip()
{
    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 200.0f, 200.0f});

    list.set_texture(xel::graphics::TextureSlot::Texture);
    auto& b0 = list.batch();
    b0.vertices.push_back({{0.0f, 0.0f}, glm::vec4{1.0f}, {0.0f, 0.0f}});
    assert(nearly(b0.scissor.z, 200.0f));

    list.push_clip({10.0f, 10.0f, 50.0f, 50.0f});
    auto& b1 = list.batch();
    b1.vertices.push_back({{0.0f, 0.0f}, glm::vec4{1.0f}, {0.0f, 0.0f}});
    assert(nearly(b1.scissor.x, 10.0f));
    assert(nearly(b1.scissor.y, 10.0f));
    assert(nearly(b1.scissor.z, 50.0f));
    assert(nearly(b1.scissor.w, 50.0f));

    list.push_clip({30.0f, 30.0f, 50.0f, 50.0f});
    auto& b2 = list.batch();
    b2.vertices.push_back({{0.0f, 0.0f}, glm::vec4{1.0f}, {0.0f, 0.0f}});
    assert(nearly(b2.scissor.x, 30.0f));
    assert(nearly(b2.scissor.y, 30.0f));
    assert(nearly(b2.scissor.z, 30.0f));
    assert(nearly(b2.scissor.w, 30.0f));

    list.pop_clip();
    list.pop_clip();
    assert(nearly(list.current_clip().z, 200.0f));
}

void test_scroll_view_clip()
{
    auto scroll = std::make_unique<xel::ui::ScrollView>();
    auto content = std::make_unique<xel::ui::ColorRect>(0, 0, 400, 400, glm::vec4{1.0f});
    scroll->set_content(std::move(content));
    scroll->measure({100.0f, 100.0f});
    scroll->arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});

    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 200.0f, 200.0f});
    scroll->append_to(list);
    assert(!list.batches().empty());
    for (const auto& batch : list.batches()) {
        assert(batch.scissor.z <= 100.0f + 1e-3f);
        assert(batch.scissor.w <= 100.0f + 1e-3f);
    }
}
} // namespace

int main()
{
    test_align_names();
    test_vbox_measure_and_padding();
    test_vbox_stretch();
    test_scroll_view();
    test_root_parent_and_dirty();
    test_rect_intersect();
    test_hbox_measure_and_spacing();
    test_hbox_space_between_and_stretch();
    test_spacer();
    test_padding_and_margin();
    test_flex_grow();
    test_min_max_size();
    test_measure_cache_invalidation();
    test_scroll_view_both_axes();
    test_draw_list_clip();
    test_scroll_view_clip();

    std::cout << "ui layout tests passed\n";
    return 0;
}
