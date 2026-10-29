#include "ui/composites/dialog.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace
{
using xel::event::InputEvent;
using xel::event::KeyCode;

InputEvent mouse(InputEvent::Type type, glm::vec2 pos)
{
    InputEvent event;
    event.type = type;
    event.mouse_pos = pos;
    return event;
}

InputEvent key(KeyCode code)
{
    InputEvent event;
    event.type = InputEvent::Type::KeyDown;
    event.key = code;
    return event;
}

glm::vec2 center(const xel::ui::Widget& widget)
{
    const auto bounds = widget.global_bounds();
    return {bounds.pos.x + bounds.size.x * 0.5f, bounds.pos.y + bounds.size.y * 0.5f};
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 1000.0f, 700.0f);
    xel::ui::UISystem ui{std::move(root)};
    auto* root_ptr = ui.root();
    ui.tick();

    // 构造对话框卡片：标题 + 正文 + 两个操作按钮。
    auto dialog = std::make_unique<xel::ui::Dialog>("Delete project?", 420.0f);
    dialog->set_message("This removes 3 keyframes. You can undo it later.");
    auto* dialog_ptr = dialog.get();

    int cancel_calls = 0;
    int delete_calls = 0;
    auto* cancel = dialog->add_action("Cancel", [&cancel_calls] { ++cancel_calls; });
    auto* confirm = dialog->add_action("Delete", [&delete_calls] { ++delete_calls; }, true);
    assert(dialog->action_count() == 2);
    assert(dialog->action(0) == cancel);
    assert(dialog->action(1) == confirm);
    assert(dialog->preferred_focus() == confirm);
    assert(dialog->message().find("keyframes") != std::string::npos);

    int close_calls = 0;
    auto* host = xel::ui::show_dialog(*root_ptr, std::move(dialog), [&close_calls] { ++close_calls; });
    ui.tick();

    assert(host != nullptr);
    assert(root_ptr->overlays().size() == 1);
    assert(root_ptr->overlays().front().get() == host);
    assert(!host->closed());
    assert(host->dialog() == dialog_ptr);

    // 对话框内容区位于窗口中央。
    const auto bounds = dialog_ptr->global_bounds();
    assert(bounds.size.x >= 420.0f - 0.5f);
    assert(bounds.pos.x > 200.0f && bounds.pos.x < 300.0f);
    assert(bounds.pos.y > 100.0f);

    // 点击遮罩关闭。
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, {6.0f, 6.0f}));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, {6.0f, 6.0f}));
    assert(close_calls == 1);
    ui.tick();
    assert(root_ptr->overlays().empty());

    // 点击操作按钮：先触发回调，再由调用方关闭。
    auto dialog2 = std::make_unique<xel::ui::Dialog>("Confirm", 360.0f);
    dialog2->set_message("Proceed?");
    xel::ui::DialogHost* host2 = nullptr;
    auto* ok = dialog2->add_action("OK", [&] {
        ++delete_calls;
        host2->close();
    }, true);
    host2 = xel::ui::show_dialog(*root_ptr, std::move(dialog2), [&close_calls] { ++close_calls; });
    ui.tick();

    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, center(*ok)));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, center(*ok)));
    assert(delete_calls == 1);
    assert(close_calls == 2);
    ui.tick();
    assert(root_ptr->overlays().empty());

    // Esc 关闭（按键从焦点控件冒泡到宿主）。
    auto dialog3 = std::make_unique<xel::ui::Dialog>("Escape test", 320.0f);
    auto* host3 = xel::ui::show_dialog(*root_ptr, std::move(dialog3), [&close_calls] { ++close_calls; });
    ui.tick();
    ui.set_focus(host3->preferred_focus());
    ui.dispatch(key(KeyCode::Escape));
    assert(host3->closed());
    assert(close_calls == 3);
    ui.tick();
    assert(root_ptr->overlays().empty());

    // 不可关闭的对话框：点击遮罩与 Esc 都无效。
    auto dialog4 = std::make_unique<xel::ui::Dialog>("Must answer", 320.0f);
    dialog4->set_dismissable(false);
    auto* host4 = xel::ui::show_dialog(*root_ptr, std::move(dialog4), [&close_calls] { ++close_calls; });
    ui.tick();
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, {6.0f, 6.0f}));
    ui.dispatch(key(KeyCode::Escape));
    assert(!host4->closed());
    assert(close_calls == 3);
    host4->close();
    ui.tick();
    assert(root_ptr->overlays().empty());
    assert(close_calls == 4);

    // 关闭事件本身能让宿主安全移除自己（不重复回调）。
    auto dialog5 = std::make_unique<xel::ui::Dialog>("Once", 300.0f);
    auto* host5 = xel::ui::show_dialog(*root_ptr, std::move(dialog5), [&close_calls] { ++close_calls; });
    ui.tick();
    host5->close();
    host5->close();
    assert(close_calls == 5);

    std::cout << "dialog tests passed\n";
    return 0;
}
