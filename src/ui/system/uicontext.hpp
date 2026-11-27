#pragma once

#include "ui/style/theme.hpp"
#include "events/key_code.hpp"

#include <cstdint>

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
//
// 主题可变性边界：
//   - 只能经 set_theme() 修改，它会推进**本 context** 的 theme_revision；
//   - 过渡缓存（Widget::StyleTransition）按这个 revision 判断是否需要重建，
//     直接改字段不会推进版本 → 会留下旧颜色的过渡缓存，所以 theme 不再公开可写；
//   - revision 挂在 context 上而不是进程全局：多个 root / lab / 测试互不打断。
struct UIContext
{
    const Theme& theme() const { return theme_; }
    void set_theme(const Theme& value)
    {
        theme_ = value;
        ++theme_revision_;
    }
    // 主题版本：任何依赖主题的派生缓存（样式过渡、测量缓存）都用它做失效键。
    std::uint64_t theme_revision() const { return theme_revision_; }

    graphics::FontAtlas* font_atlas = nullptr;
    InputMode input_mode = InputMode::Mouse;
    // 最近一次输入事件携带的修饰键：控件不能在鼠标回调里拿到 mods，
    // 需要 Shift/Ctrl 参与判定时读这里（例如时间轴的追加选择）。
    event::Modifier modifiers = event::Modifier::None;

private:
    Theme theme_ = Theme::dark();
    std::uint64_t theme_revision_ = 1;
};
} // namespace xel::ui
