#pragma once

#include "theme.hpp"

namespace xel::graphics
{
class FontAtlas;
}

namespace xel::ui
{
// UI 树共享的资源入口：主题与字体图集不拥有资源，只提供访问。
struct UIContext
{
    Theme theme = Theme::dark();
    graphics::FontAtlas* font_atlas = nullptr;
};
} // namespace xel::ui
