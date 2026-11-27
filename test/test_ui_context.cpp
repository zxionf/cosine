#include "graphics/font_atlas.hpp"
#include "ui/core/label.hpp"
#include "ui/containers/panel.hpp"
#include "ui/system/root.hpp"
#include "ui/system/uicontext.hpp"
#include "ui/containers/vbox.hpp"

#include <cassert>
#include <iostream>
#include <memory>

int main()
{
    xel::graphics::FontAtlas atlas;

    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    // UI-42：主题只能经 set_theme() 修改（它会推进 context 的 theme revision）。
    {
        xel::ui::Theme t = root->context()->theme();
        t.text_size = 32.0f;
        t.padding = 20.0f;
        root->set_theme(t);
    }
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
    {
        xel::ui::Theme t = other.theme();
        t.text_size = 12.0f;
        other.set_theme(t);
    }
    root->set_context(&other);
    assert(label_ptr->context() == &other);
    assert(label_ptr->text_size() == 12.0f);
    assert(label_ptr->font_atlas() == nullptr);

    // 显式 set_padding 锁住内边距：主题重刷不再覆盖（CO-7）。
    auto pinned = std::make_unique<xel::ui::Panel>(0.0f, 0.0f, 100.0f, 50.0f);
    pinned->set_padding(0.0f);
    auto* pinned_ptr = pinned.get();
    vbox_ptr->add(std::move(pinned));
    assert(pinned_ptr->padding_override());
    assert(pinned_ptr->padding().left == 0.0f);

    xel::ui::UIContext third;
    {
        xel::ui::Theme t = third.theme();
        t.padding = 30.0f;
        t.spacing = 11.0f;
        third.set_theme(t);
    }
    root->set_context(&third);
    assert(pinned_ptr->padding().left == 0.0f);   // 显式值优先，不被主题掤回
    assert(panel_ptr->padding().left == 30.0f);   // 未显式设过的面板跟随主题

    // UI-35：裸容器（Vbox/Hbox/Grid/Wrap）也接主题 padding/spacing；显式值仍然优先。
    assert(vbox_ptr->padding().left == 30.0f);
    assert(!vbox_ptr->padding_override());
    auto pinned_box = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 100.0f, 50.0f);
    pinned_box->set_padding(0.0f);
    pinned_box->set_spacing(0.0f);
    auto* pinned_box_ptr = pinned_box.get();
    vbox_ptr->add(std::move(pinned_box));
    assert(pinned_box_ptr->padding_override() && pinned_box_ptr->spacing_override());

    xel::ui::UIContext fourth;
    {
        xel::ui::Theme t = fourth.theme();
        t.padding = 14.0f;
        t.spacing = 9.0f;
        fourth.set_theme(t);
    }
    root->set_context(&fourth);
    assert(vbox_ptr->padding().left == 14.0f);
    assert(pinned_box_ptr->padding().left == 0.0f);
    assert(pinned_box_ptr->spacing_override());

    std::cout << "ui context tests passed\n";
    return 0;
}
