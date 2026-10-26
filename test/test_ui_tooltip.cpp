#include "ui/tooltip.hpp"
#include "ui/root.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 800.0f, 600.0f);
    auto tooltip = std::make_unique<xel::ui::Tooltip>("hint text");

    auto* tip = tooltip.get();
    root->add_overlay(std::move(tooltip));
    root->arrange({{0.0f, 0.0f}, {800.0f, 600.0f}});

    const auto bounds = root->global_bounds();
    assert(bounds.size.x == 800.0f);

    // delay 默认 0.35s：累计不足不显示。
    tip->set_delay(0.35f);
    tip->update_hover(0.1f, true, {100.0f, 100.0f}, bounds);
    assert(!tip->visible());
    tip->update_hover(0.2f, true, {100.0f, 100.0f}, bounds);
    assert(!tip->visible());
    tip->update_hover(0.1f, true, {100.0f, 100.0f}, bounds);
    assert(tip->visible());

    // 跟随光标：右下偏移。
    tip->update_hover(0.016f, true, {200.0f, 150.0f}, bounds);
    root->layout();
    auto global = tip->global_bounds();
    assert(global.pos.x > 200.0f);
    assert(global.pos.y > 150.0f);

    // 靠近右下角时翻到左上方，且不越界。
    tip->update_hover(0.016f, true, {795.0f, 595.0f}, bounds);
    root->layout();
    global = tip->global_bounds();
    assert(global.pos.x < 795.0f);
    assert(global.pos.y < 595.0f);
    assert(global.pos.x + global.size.x <= bounds.pos.x + bounds.size.x + 0.5f);
    assert(global.pos.y + global.size.y <= bounds.pos.y + bounds.size.y + 0.5f);

    // 离开目标立即隐藏，并清零计时。
    tip->update_hover(0.016f, false, {795.0f, 595.0f}, bounds);
    assert(!tip->visible());
    tip->update_hover(0.1f, true, {100.0f, 100.0f}, bounds);
    assert(!tip->visible());
    tip->update_hover(0.3f, true, {100.0f, 100.0f}, bounds);
    assert(tip->visible());

    // delay = 0 时立即显示。
    tip->set_delay(0.0f);
    tip->hide();
    tip->update_hover(0.0f, true, {300.0f, 200.0f}, bounds);
    assert(tip->visible());

    // 空 bounds 时不做翻转，只按偏移定位。
    tip->update_hover(0.0f, true, {795.0f, 595.0f},
                      {{0.0f, 0.0f}, {0.0f, 0.0f}});
    root->layout();
    global = tip->global_bounds();
    assert(global.pos.x > 795.0f);
    assert(global.pos.y > 595.0f);

    // 尺寸随文本变化。
    const float before = tip->measure({}).x;
    tip->set_text("a much longer hint text");
    root->layout();
    assert(tip->measure({}).x > before);

    std::cout << "tooltip tests passed\n";
    return 0;
}
