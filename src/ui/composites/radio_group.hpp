#pragma once

#include "ui/theme.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/radio.hpp"

#include <functional>
#include <string>
#include <vector>

namespace xel::ui
{
// 单选组：内部维护若干 Radio，保证同时只有一个选中。
class RadioGroup : public Vbox
{
public:
    explicit RadioGroup(float width = 200.0f)
        : Vbox(0.0f, 0.0f, width, 0.0f)
    {
        set_spacing(ui_theme().spacing);
        set_cross_align(Align::top_left());
    }

    void set_options(std::vector<std::string> options, int selected = 0)
    {
        clear_children();
        radios_.clear();
        for (std::size_t i = 0; i < options.size(); ++i) {
            auto radio = std::make_unique<Radio>(options[i], false, [this, i]() {
                select(static_cast<int>(i), true);
            });
            // roving tabindex：整个组只占一个 Tab 停靠点，组内用方向键切换。
            radio->set_tab_order(-1);
            radios_.push_back(radio.get());
            add(std::move(radio));
        }
        selected_ = -1;
        select(selected, false);
    }

    int selected() const { return selected_; }
    std::size_t option_count() const { return radios_.size(); }
    Radio* option(std::size_t index)
    {
        return index < radios_.size() ? radios_[index] : nullptr;
    }

    void select(int index, bool notify = true)
    {
        if (radios_.empty()) return;
        const int clamped = std::clamp(index, 0, static_cast<int>(radios_.size()) - 1);
        if (clamped == selected_) return;
        selected_ = clamped;
        for (std::size_t i = 0; i < radios_.size(); ++i) {
            radios_[i]->set_selected(static_cast<int>(i) == selected_);
        }
        if (notify && on_change_) on_change_(selected_);
    }

    void set_on_change(std::function<void(int)> handler) { on_change_ = std::move(handler); }

    // 组成一个 Tab 停靠点（子 Radio 的 tab_order 为 -1）。
    bool focusable() const override { return !radios_.empty(); }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (radios_.empty()) return false;
        const int count = static_cast<int>(radios_.size());
        const int base = selected_ < 0 ? 0 : selected_;
        int next = base;
        switch (key) {
            case event::KeyCode::Up:
            case event::KeyCode::Left:
                next = (base + count - 1) % count;
                break;
            case event::KeyCode::Down:
            case event::KeyCode::Right:
                next = (base + 1) % count;
                break;
            case event::KeyCode::Home:
                next = 0;
                break;
            case event::KeyCode::End:
                next = count - 1;
                break;
            default:
                return false;
        }
        select(next, true);
        sync_group_focus();
        return true;
    }

    void on_focus_gained() override
    {
        Widget::on_focus_gained();
        sync_group_focus();
    }

    void on_focus_lost() override
    {
        Widget::on_focus_lost();
        clear_group_focus();
    }

private:
    // 焦点框由选中的 Radio 自己绘制，所以只在选中项上打焦点标记。
    void sync_group_focus()
    {
        for (std::size_t i = 0; i < radios_.size(); ++i) {
            radios_[i]->set_focused(is_focused() && static_cast<int>(i) == selected_);
        }
    }

    void clear_group_focus()
    {
        for (auto* radio : radios_) radio->set_focused(false);
    }

    std::vector<Radio*> radios_;
    int selected_ = -1;
    std::function<void(int)> on_change_;
};
} // namespace xel::ui
