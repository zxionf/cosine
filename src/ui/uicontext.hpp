#pragma once

#include "theme.hpp"
#include "../events/key_code.hpp"

namespace xel::graphics
{
class FontAtlas;
}

namespace xel::ui
{
// 最近一次输入来源：焦点环只在键盘操作时显示，鼠标点击聚焦不画。
enum class InputMode
{
    Mouse,
    Keyboard,
};

// UI 树共享的资源入口：主题与字体图集不拥有资源，只提供访问。
struct UIContext
{
    Theme theme = Theme::dark();
    graphics::FontAtlas* font_atlas = nullptr;
    InputMode input_mode = InputMode::Mouse;
    // 最近一次输入事件携带的修饰键：控件不能在鼠标回调里拿到 mods，
    // 需要 Shift/Ctrl 参与判定时读这里（例如时间轴的追加选择）。
    event::Modifier modifiers = event::Modifier::None;
};
} // namespace xel::ui
