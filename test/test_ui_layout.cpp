#include "ui/align.hpp"
#include "ui/color_rect.hpp"
#include "ui/root.hpp"
#include "ui/scroll_view.hpp"
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
    assert(nearly(scroll->scroll_offset(), 0.0f));

    assert(scroll->on_scroll({10.0f, 10.0f}, -1.0f));
    assert(nearly(scroll->scroll_offset(), 40.0f));

    assert(scroll->on_scroll({10.0f, 10.0f}, 1.0f));
    assert(nearly(scroll->scroll_offset(), 0.0f));

    // 超出 viewport 不消费
    assert(!scroll->on_scroll({1000.0f, 1000.0f}, -1.0f));
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
} // namespace

int main()
{
    test_align_names();
    test_vbox_measure_and_padding();
    test_vbox_stretch();
    test_scroll_view();
    test_root_parent_and_dirty();
    test_rect_intersect();

    std::cout << "ui layout tests passed\n";
    return 0;
}
