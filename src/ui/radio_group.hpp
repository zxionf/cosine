#pragma once

#include "theme.hpp"
#include "vbox.hpp"
#include "radio.hpp"

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

private:
    std::vector<Radio*> radios_;
    int selected_ = -1;
    std::function<void(int)> on_change_;
};
} // namespace xel::ui
