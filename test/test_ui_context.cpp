#include "graphics/font_atlas.hpp"
#include "ui/core/label.hpp"
#include "ui/containers/panel.hpp"
#include "ui/root.hpp"
#include "ui/uicontext.hpp"
#include "ui/containers/vbox.hpp"

#include <cassert>
#include <iostream>
#include <memory>

int main()
{
    xel::graphics::FontAtlas atlas;

    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    root->context()->theme.text_size = 32.0f;
    root->context()->theme.padding = 20.0f;
    root->context()->font_atlas = &atlas;

    auto vbox = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 300.0f, 200.0f);
    auto panel = std::make_unique<xel::ui::Panel>(0.0f, 0.0f, 200.0f, 100.0f);
    auto label = std::make_unique<xel::ui::Label>("Hello");

    auto* vbox_ptr = vbox.get();
    auto* panel_ptr = panel.get();
    auto* label_ptr = label.get();

    panel->add(std::move(label));
    vbox->add(std::move(panel));
    root->add(std::move(vbox));

    assert(vbox_ptr->context() == root->context());
    assert(panel_ptr->context() == root->context());
    assert(label_ptr->context() == root->context());
    assert(label_ptr->ui_theme().text_size == 32.0f);
    assert(label_ptr->font_atlas() == &atlas);
    assert(label_ptr->text_size() == 32.0f);
    assert(panel_ptr->padding().left == 20.0f);

    xel::ui::UIContext other;
    other.theme.text_size = 12.0f;
    root->set_context(&other);
    assert(label_ptr->context() == &other);
    assert(label_ptr->text_size() == 12.0f);
    assert(label_ptr->font_atlas() == nullptr);

    std::cout << "ui context tests passed\n";
    return 0;
}
