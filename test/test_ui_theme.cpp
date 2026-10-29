#include "ui/core/label.hpp"
#include "ui/containers/panel.hpp"
#include "ui/root.hpp"
#include "ui/theme.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

int main()
{
    xel::ui::Theme dark = xel::ui::Theme::dark();
    xel::ui::Theme light = xel::ui::Theme::light();
    assert(std::strcmp(dark.name, "dark") == 0);
    assert(std::strcmp(light.name, "light") == 0);
    assert(light.text.x < dark.text.x);           // 亮色主题文字更深
    assert(light.panel_bg.x > dark.panel_bg.x);   // 亮色主题面板更亮

    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    auto panel = std::make_unique<xel::ui::Panel>(0.0f, 0.0f, 300.0f, 200.0f);
    auto label = std::make_unique<xel::ui::Label>("theme");
    auto* panel_ptr = panel.get();
    auto* label_ptr = label.get();
    panel->add(std::move(label));
    root->add(std::move(panel));

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(std::strcmp(ui.root()->ui_theme().name, "dark") == 0);
    const float dark_padding = panel_ptr->padding().left;
    const float dark_text = label_ptr->text_size();

    // 自定义主题：字号与 padding 都变化，用于验证缓存刷新。
    xel::ui::Theme custom = xel::ui::Theme::light();
    custom.padding = 14.0f;
    custom.text_size = 20.0f;
    ui.set_theme(custom);

    assert(std::strcmp(ui.root()->ui_theme().name, "light") == 0);
    assert(panel_ptr->padding().left == 14.0f);
    assert(panel_ptr->padding().left != dark_padding);
    assert(label_ptr->text_size() == 20.0f);
    assert(label_ptr->text_size() != dark_text);
    assert(label_ptr->text_color() == custom.text);

    // 切回深色。
    ui.set_theme(xel::ui::Theme::dark());
    assert(std::strcmp(ui.root()->ui_theme().name, "dark") == 0);
    assert(panel_ptr->padding().left == xel::ui::Theme::dark().padding);
    assert(label_ptr->text_size() == xel::ui::Theme::dark().text_size);

    std::cout << "ui theme tests passed\n";
    return 0;
}
