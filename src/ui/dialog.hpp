#pragma once

#include "button.hpp"
#include "hbox.hpp"
#include "label.hpp"
#include "panel.hpp"
#include "root.hpp"
#include "scrim.hpp"
#include "theme.hpp"
#include "vbox.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
class DialogHost;

// 对话框卡片：标题 + 内容区 + 右下角操作按钮。
class Dialog : public Panel
{
public:
    explicit Dialog(std::string title = "Dialog", float width = 420.0f)
        : Panel(0.0f, 0.0f, width, 0.0f)
    {
        set_min_size({width, 0.0f});
        set_max_size({width, 1.0e9f});
        set_cross_align(Align::top_left());

        auto title_label = std::make_unique<Label>(std::move(title), 20.0f);
        title_ = title_label.get();
        Panel::add(std::move(title_label));

        auto body = std::make_unique<Vbox>(0.0f, 0.0f, width, 0.0f);
        body->set_cross_align(Align::top_left());
        body->set_padding(0.0f);
        body_ = body.get();
        Panel::add(std::move(body));

        auto actions = std::make_unique<Hbox>(0.0f, 0.0f, width, 0.0f);
        actions->set_main_align(MainAlign::End);
        actions->set_align(Align::h_stretch());
        actions->set_padding(0.0f);
        actions_ = actions.get();
        Panel::add(std::move(actions));
    }

    void set_title(std::string title)
    {
        if (title_) title_->set_text(std::move(title));
    }
    const std::string& title() const
    {
        static const std::string empty;
        return title_ ? title_->text() : empty;
    }

    // 内容进入正文区。
    void add(std::unique_ptr<Widget> child) override { body_->add(std::move(child)); }

    // 便捷：正文加一段文字。
    void set_message(std::string message)
    {
        if (!message_) {
            auto label = std::make_unique<Label>(std::move(message), 14.0f);
            label->set_wrap(true);
            label->set_max_size({width() - padding_left() - padding_right(), 1.0e9f});
            message_ = label.get();
            body_->add(std::move(label));
            return;
        }
        message_->set_text(std::move(message));
    }
    const std::string& message() const
    {
        static const std::string empty;
        return message_ ? message_->text() : empty;
    }

    // 添加操作按钮；primary 用来标记主要动作（仅用于视觉强调位）。
    Button* add_action(std::string label, std::function<void()> handler, bool primary = false)
    {
        auto button = std::make_unique<Button>(std::move(label), std::move(handler));
        auto* raw = button.get();
        if (primary) primary_ = raw;
        actions_->add(std::move(button));
        mark_layout_dirty();
        return raw;
    }

    std::size_t action_count() const { return actions_->children().size(); }
    Button* action(std::size_t index) const
    {
        if (index >= actions_->children().size()) return nullptr;
        return dynamic_cast<Button*>(actions_->children()[index].get());
    }

    // 打开时可用来设置初始焦点：优先主按钮，否则第一个按钮。
    Widget* preferred_focus() const
    {
        if (primary_) return primary_;
        return action_count() > 0 ? action(0) : nullptr;
    }

    void set_dismissable(bool dismissable) { dismissable_ = dismissable; }
    bool dismissable() const { return dismissable_; }

    void set_on_dismiss(std::function<void()> handler) { on_dismiss_ = std::move(handler); }

    // 由宿主调用：对话框请求关闭（Esc / 点击遮罩 / 按钮回调里调用）。
    void dismiss()
    {
        if (on_dismiss_) on_dismiss_();
    }

    Vbox* body() { return body_; }
    Hbox* actions() { return actions_; }

private:
    Label* title_ = nullptr;
    Label* message_ = nullptr;
    Vbox* body_ = nullptr;
    Hbox* actions_ = nullptr;
    Button* primary_ = nullptr;
    bool dismissable_ = true;
    std::function<void()> on_dismiss_;
};

// 模态层：全屏遮罩 + 居中对话框。作为 overlay 添加到 UIRoot。
class DialogHost : public Widget
{
public:
    explicit DialogHost(std::unique_ptr<Dialog> dialog)
    {
        dialog_ = dialog.get();
        dialog->set_on_dismiss([this] { close(); });
        adopt(std::move(dialog));
    }

    Dialog* dialog() const { return dialog_; }

    // 打开时建议聚焦的控件：优先对话框内的主按钮，否则聚焦宿主自身（保证 Esc 可用）。
    Widget* preferred_focus()
    {
        if (dialog_) {
            if (auto* widget = dialog_->preferred_focus()) return widget;
        }
        return this;
    }
    bool focusable() const override { return true; }

    void set_on_close(std::function<void()> handler) { on_close_ = std::move(handler); }

    // 关闭并从 root.overlays 中移除（延迟销毁，事件派发期间安全）。
    void close()
    {
        if (closed_) return;
        closed_ = true;
        if (auto* root = find_root()) root->remove_overlay(this);
        if (on_close_) on_close_();
    }
    bool closed() const { return closed_; }

    glm::vec2 measure(glm::vec2 constraints) override { return constraints; }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        if (!dialog_) return;

        const auto& t = ui_theme();
        const float margin = 24.0f;
        const float max_w = std::max(120.0f, slot.size.x - margin * 2.0f);
        const float max_h = std::max(120.0f, slot.size.y - margin * 2.0f);
        glm::vec2 size = dialog_->measure({max_w, max_h});
        size.x = std::clamp(size.x, 120.0f, max_w);
        size.y = std::clamp(size.y, 80.0f, max_h);

        const glm::vec2 pos{
            slot.pos.x + (slot.size.x - size.x) * 0.5f,
            slot.pos.y + (slot.size.y - size.y) * 0.5f
        };
        dialog_->arrange({pos, size});
        (void)t;
    }

    void append_to(graphics::DrawList& list) const override
    {
        // 遮罩绘制与 Scrim 共用一份实现。
        append_scrim(list, global_, 0.55f);

        if (!dialog_) return;
        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});
        dialog_->append_to(list);
        list.pop_clip();
    }

    bool hit_testable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int) override
    {
        if (!dialog_) return true;
        if (!dialog_->global_bounds().contains(pos) && dialog_->dismissable()) {
            dialog_->dismiss();
        }
        // 无论命中与否都吞掉点击，避免穿透到下层控件。
        return true;
    }

    bool on_click(glm::vec2, int) override { return true; }
    bool on_scroll(glm::vec2, glm::vec2) override { return true; }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (key == event::KeyCode::Escape && dialog_ && dialog_->dismissable()) {
            dialog_->dismiss();
            return true;
        }
        return false;
    }

    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

private:
    UIRoot* find_root() const
    {
        for (Widget* widget = const_cast<DialogHost*>(this); widget != nullptr; widget = widget->parent()) {
            if (auto* root = dynamic_cast<UIRoot*>(widget)) return root;
        }
        return nullptr;
    }

    Dialog* dialog_ = nullptr;
    std::function<void()> on_close_;
    bool closed_ = false;
};

// 便捷入口：创建对话框并作为 overlay 挂到 root。
inline DialogHost* show_dialog(UIRoot& root, std::unique_ptr<Dialog> dialog,
                               std::function<void()> on_close = {})
{
    auto host = std::make_unique<DialogHost>(std::move(dialog));
    auto* raw = host.get();
    raw->set_on_close(std::move(on_close));
    root.add_overlay(std::move(host));
    return raw;
}
} // namespace xel::ui
