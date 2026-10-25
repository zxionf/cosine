#include "ui_demo.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1200.0f, 800.0f);
    auto lab = xel::demo::build(*root);

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(!lab.samples.empty());
    assert(lab.info != nullptr);
    assert(lab.overlay != nullptr);
    assert(!lab.overlay->visible());

    // 初始状态：调试层全关、像素对齐开启。
    for (const auto* label : lab.samples) {
        assert(!label->text_debug().any());
        assert(label->pixel_snap());
    }
    assert(lab.info->text().find("glyphs: off") != std::string::npos);
    assert(lab.info->text().find("snap: on") != std::string::npos);

    lab.toggle_glyph_boxes();
    assert(lab.options.glyph_boxes);
    for (const auto* label : lab.samples) assert(label->text_debug().glyph_boxes);
    assert(lab.info->text().find("glyphs: on") != std::string::npos);

    lab.toggle_line_box();
    assert(lab.options.line_box);
    assert(lab.options.baseline);
    for (const auto* label : lab.samples) {
        assert(label->text_debug().line_box);
        assert(label->text_debug().baseline);
    }

    lab.toggle_advance();
    for (const auto* label : lab.samples) assert(label->text_debug().advance);

    lab.toggle_pixel_snap();
    assert(!lab.pixel_snap);
    for (const auto* label : lab.samples) assert(!label->pixel_snap());
    assert(lab.info->text().find("snap: off") != std::string::npos);

    // 关闭后重新开启，回到干净状态。
    lab.toggle_glyph_boxes();
    lab.toggle_line_box();
    lab.toggle_advance();
    for (const auto* label : lab.samples) assert(!label->text_debug().any());

    lab.overlay->set_visible(true);
    assert(lab.overlay->visible());

    std::cout << "ui demo tests passed\n";
    return 0;
}
