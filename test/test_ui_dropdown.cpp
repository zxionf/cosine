#include "ui/composites/dropdown.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/root.hpp"
#include "ui/ui_system.hpp"
#include "ui/containers/vbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
using xel::event::KeyCode;
using xel::event::Modifier;
} // namespace

int main()
{
    // ---------- 基本状态 ----------
    {
        xel::ui::Dropdown dropdown({"Small", "Medium", "Large"}, 1);
        assert(dropdown.selected() == 1);
        assert(dropdown.selected_text() == "Medium");
        assert(!dropdown.is_open());
        assert(dropdown.measure({200.0f, 30.0f}).y > 0.0f);
    }

    // ---------- 打开 / 键盘选择 / 关闭 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 600.0f, 400.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>(std::vector<std::string>{"A", "B", "C"}, 0);
        auto* dd = dropdown.get();
        int changes = 0;
        int last = -1;
        dd->set_on_change([&](int index) { ++changes; last = index; });
        dd->set_pos(40.0f, 40.0f);
        root->add(std::move(dropdown));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        const std::size_t base = ui.root()->overlays().size();

        // 焦点转移必须真正清掉 focused：否则 Dropdown 选完后边框会一直是强调色。
        ui.set_focus(dd);
        assert(dd->is_focused());
        ui.set_focus(nullptr);
        assert(!dd->is_focused());

        dd->open();
        ui.tick();
        assert(dd->is_open());
        assert(ui.root()->overlays().size() == base + 2); // scrim + list
        assert(dd->highlight() == 0);

        assert(dd->on_key_down(KeyCode::Down, Modifier::None));
        assert(dd->highlight() == 1);
        assert(dd->on_key_down(KeyCode::End, Modifier::None));
        assert(dd->highlight() == 2);
        assert(dd->on_key_down(KeyCode::Home, Modifier::None));
        assert(dd->highlight() == 0);
        assert(dd->on_key_down(KeyCode::Up, Modifier::None));
        assert(dd->highlight() == 0); // clamp

        assert(dd->on_key_down(KeyCode::Down, Modifier::None));
        assert(dd->on_key_down(KeyCode::Enter, Modifier::None));
        assert(!dd->is_open());
        assert(dd->selected() == 1);
        assert(changes == 1);
        assert(last == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // Esc 关闭且不改变选择。
        dd->open();
        ui.tick();
        assert(dd->is_open());
        assert(dd->on_key_down(KeyCode::Escape, Modifier::None));
        assert(!dd->is_open());
        assert(dd->selected() == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // Tab 关闭，不消费焦点事件。
        dd->open();
        ui.tick();
        assert(!dd->on_key_down(KeyCode::Tab, Modifier::None));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 点击遮罩关闭。
        dd->open();
        ui.tick();
        assert(ui.root()->overlays().size() == base + 2);
        auto* scrim = ui.root()->overlays()[base].get();
        assert(scrim->on_mouse_down({10.0f, 10.0f}, 0));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 点击列表选项提交。
        dd->open();
        ui.tick();
        assert(ui.root()->overlays().size() == base + 2);
        auto* list = ui.root()->overlays()[base + 1].get();
        const auto bounds = list->global_bounds();
        assert(bounds.size.x >= 160.0f);
        assert(bounds.size.y > 0.0f);

        // 弹层上下各留一份 padding：高度 = 行高 × 项数 + padding × 2（与 MenuBar 一致）。
        const auto drop_theme = xel::ui::Theme::dark();
        const float pad = drop_theme.padding;
        const float row = drop_theme.control_height;
        assert(std::fabs(bounds.size.y - (row * 3.0f + pad * 2.0f)) < 0.01f);

        assert(list->on_click({bounds.pos.x + 4.0f, bounds.pos.y + pad + row * 0.5f}, 0));
        assert(dd->selected() == 0);
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);

        // 列表行悬停高亮。
        dd->open();
        ui.tick();
        list = ui.root()->overlays()[base + 1].get();
        const auto list_bounds = list->global_bounds();
        const float second_row_y = list_bounds.pos.y + pad + row * 1.5f;
        assert(list->on_mouse_move({list_bounds.pos.x + 4.0f, second_row_y}));
        assert(list->on_click({list_bounds.pos.x + 4.0f, second_row_y}, 0));
        assert(dd->selected() == 1);
        ui.tick();
        assert(ui.root()->overlays().size() == base);
    }

    // ---------- 再次点击自身按钮关闭 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>(std::vector<std::string>{"A", "B"}, 0);
        auto* dd = dropdown.get();
        dd->set_pos(20.0f, 20.0f);
        root->add(std::move(dropdown));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();
        const auto base = ui.root()->overlays().size();

        assert(dd->on_mouse_down({30.0f, 30.0f}, 0));
        ui.tick();
        assert(dd->is_open());
        assert(ui.root()->overlays().size() == base + 2);
        assert(dd->on_mouse_down({30.0f, 30.0f}, 0));
        assert(!dd->is_open());
        ui.tick();
        assert(ui.root()->overlays().size() == base);
    }

    // ---------- 空选项不展开 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>();
        auto* dd = dropdown.get();
        root->add(std::move(dropdown));
        xel::ui::UISystem ui{std::move(root)};
        ui.tick();
        const auto before = ui.root()->overlays().size();
        dd->open();
        ui.tick();
        assert(!dd->is_open());
        assert(ui.root()->overlays().size() == before);
    }

    // ---------- 回归：通过事件系统点击下拉项后不悬垂 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 600.0f, 400.0f);
        auto dropdown = std::make_unique<xel::ui::Dropdown>(std::vector<std::string>{"A", "B", "C"}, 0);
        auto* dd = dropdown.get();
        dd->set_pos(40.0f, 40.0f);
        root->add(std::move(dropdown));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        auto mouse = [](xel::event::InputEvent::Type type, glm::vec2 pos) {
            xel::event::InputEvent e;
            e.type = type;
            e.mouse_pos = pos;
            return e;
        };

        // 悬停并点击按钮 → 展开
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseMove, {60.0f, 55.0f}));
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, {60.0f, 55.0f}));
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, {60.0f, 55.0f}));
        ui.tick();
        assert(dd->is_open());
        assert(ui.hovered() == dd);

        // 悬停到列表第二行
        auto* list = ui.root()->overlays().back().get();
        const auto bounds = list->global_bounds();
        const glm::vec2 row{bounds.pos.x + 8.0f, bounds.pos.y + 40.0f};
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseMove, row));
        ui.tick();
        assert(ui.hovered() == list);

        // 点击该行：提交并移除 overlay（此时 hovered_ 仍指向列表）
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonDown, row));
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseButtonUp, row));
        assert(dd->selected() == 1);
        assert(!dd->is_open());

        // tick 会销毁已移除的 overlay，并把悬垂的 hover 目标清掉。
        ui.tick();
        assert(ui.hovered() == nullptr);

        // 再次移动鼠标不能崩溃。
        ui.dispatch(mouse(xel::event::InputEvent::Type::MouseMove, {300.0f, 300.0f}));
        assert(ui.hovered() == nullptr);
        ui.tick();
    }

    // ---------- 回归：清空子项后 hover 目标不能悬垂 ----------
    {
        auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
        auto box = std::make_unique<xel::ui::Vbox>(0.0f, 0.0f, 300.0f, 200.0f);
        auto* box_ptr = box.get();
        auto rect = std::make_unique<xel::ui::ColorRect>(0.0f, 0.0f, 120.0f, 40.0f, glm::vec4{1.0f});
        auto* rect_ptr = rect.get();
        box->add(std::move(rect));
        root->add(std::move(box));

        xel::ui::UISystem ui{std::move(root)};
        ui.tick();

        xel::event::InputEvent move;
        move.type = xel::event::InputEvent::Type::MouseMove;
        move.mouse_pos = {20.0f, 20.0f};
        ui.dispatch(move);
        assert(ui.hovered() == rect_ptr);

        // 清空子项：下一次 tick 会销毁控件，hover 目标需要被清理。
        box_ptr->clear_children();
        ui.tick();
        assert(ui.hovered() == nullptr);

        ui.dispatch(move);
        assert(ui.hovered() == nullptr || ui.hovered() == box_ptr);
    }
    std::cout << "ui dropdown tests passed\n";
    return 0;
}
