#include "ui_gallery.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace
{
glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.global_bounds();
    return {bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f};
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1200.0f, 800.0f);
    auto gallery = xel::demo::build(*root);

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(gallery.sections.size() == 6);
    assert(gallery.nav_buttons.size() == 6);
    assert(gallery.sections[0]->visible());
    assert(!gallery.sections[3]->visible());

    gallery.show_section(3);
    assert(!gallery.sections[0]->visible());
    assert(gallery.sections[3]->visible());
    assert(gallery.nav_buttons[3]->label().rfind(">", 0) == 0);

    gallery.show_section(0);
    assert(gallery.nav_buttons[0]->label().rfind(">", 0) == 0);
    assert(gallery.nav_buttons[3]->label().rfind(">", 0) != 0);

    // ScrollView 内容大于 viewport，裁剪与滚动状态可读。
    assert(gallery.scroll_vertical->content_size().y > gallery.scroll_vertical->viewport_size().y);
    assert(gallery.scroll_both->content_size().x > gallery.scroll_both->viewport_size().x);

    // Tooltip 跟随 hover 目标显示/隐藏。
    assert(!gallery.tooltip->visible());
    gallery.update_tooltip(glm::vec2{-1000.0f, -1000.0f});
    assert(!gallery.tooltip->visible());

    gallery.update_tooltip(center(*gallery.tip_watermark));
    assert(gallery.tooltip->visible());
    assert(!gallery.tooltip->text().empty());

    gallery.update_tooltip(glm::vec2{-1000.0f, -1000.0f});
    assert(!gallery.tooltip->visible());

    // 调试面板默认隐藏。
    assert(!gallery.debug->visible());
    gallery.debug->toggle();
    assert(gallery.debug->visible());

    // 分区切换后仍然只有当前分区可见。
    gallery.show_section(5);
    for (std::size_t i = 0; i < gallery.sections.size(); ++i) {
        assert(gallery.sections[i]->visible() == (i == 5));
    }

    std::cout << "ui gallery tests passed\n";
    return 0;
}
