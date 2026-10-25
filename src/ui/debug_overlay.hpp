#pragma once

#include "button.hpp"
#include "checkbox.hpp"
#include "color_rect.hpp"
#include "hbox.hpp"
#include "label.hpp"
#include "panel.hpp"
#include "scroll_view.hpp"
#include "slider.hpp"
#include "spacer.hpp"
#include "splitter.hpp"
#include "text_input.hpp"
#include "tooltip.hpp"
#include "vbox.hpp"
#include "../graphics/frame_stats.hpp"

#include <format>
#include <string>

namespace xel::ui
{
class DebugOverlay : public Panel
{
public:
    DebugOverlay()
        : Panel(12.0f, 12.0f, 320.0f, 0.0f)
    {
        auto label = std::make_unique<Label>("");
        label_ = label.get();
        add(std::move(label));
        set_max_size({420.0f, 400.0f});
        set_visible(false);
    }

    bool hit_testable() const override { return false; }

    void toggle() { set_visible(!visible()); }

    void update(const graphics::FrameStats& stats, const Widget* hovered, const Widget* focused, glm::vec2 scroll)
    {
        label_->set_text(std::format(
            "FPS: {:.1f}\nframe: {:.2f} ms\n"
            "draw calls: {}\nbatches: {}\nvertices: {}\nindices: {}\nclipped: {}\n"
            "hover: {}\nfocus: {}\nscroll: {:.0f}, {:.0f}",
            stats.fps,
            stats.frame_time_ms,
            stats.draw.draw_calls,
            stats.draw.batches,
            stats.draw.vertices,
            stats.draw.indices,
            stats.draw.clipped_batches,
            widget_name(hovered),
            widget_name(focused),
            scroll.x,
            scroll.y));
    }

    const std::string& text() const { return label_->text(); }

private:
    static std::string widget_name(const Widget* widget)
    {
        if (!widget) return "-";
        if (dynamic_cast<const Label*>(widget)) return "Label";
        if (dynamic_cast<const Button*>(widget)) return "Button";
        if (dynamic_cast<const Checkbox*>(widget)) return "Checkbox";
        if (dynamic_cast<const Slider*>(widget)) return "Slider";
        if (dynamic_cast<const TextInput*>(widget)) return "TextInput";
        if (dynamic_cast<const Tooltip*>(widget)) return "Tooltip";
        if (dynamic_cast<const ScrollView*>(widget)) return "ScrollView";
        if (dynamic_cast<const Panel*>(widget)) return "Panel";
        if (dynamic_cast<const Vbox*>(widget)) return "Vbox";
        if (dynamic_cast<const Hbox*>(widget)) return "Hbox";
        if (dynamic_cast<const ColorRect*>(widget)) return "ColorRect";
        if (dynamic_cast<const Spacer*>(widget)) return "Spacer";
        if (dynamic_cast<const Splitter*>(widget)) return "Splitter";
        return "Widget";
    }

    Label* label_ = nullptr;
};
} // namespace xel::ui
