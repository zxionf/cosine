#pragma once

namespace xel::ui
{
// 四边间距（UI-48）：从 widget.hpp 搬出，保持 `ui::Edges{l,t,r,b}` 的既有写法。
// 聚合初始化顺序固定为 left / top / right / bottom。
struct Edges
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

// 盒模型（UI-48）：内边距 / 外边距 + 「内边距是否被显式锁定」。
//
// 语义沿用 UI-35：set_padding() 是调用方的显式意图（锁定），apply_theme_padding()
// 只在未锁定时生效；所以 padding_override 必须和内边距放在一起。
struct BoxMetrics
{
    Edges padding{};
    bool padding_override = false;
    Edges margin{};
};
} // namespace xel::ui
