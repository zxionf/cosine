#pragma once

namespace xel::ui
{
// 四边间距。聚合初始化顺序固定为 left / top / right / bottom。
struct Edges
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

// 盒模型：内边距 / 外边距 + 「内边距是否被显式锁定」。
//
// set_padding() 是调用方的显式意图（锁定），apply_theme_padding() 只在未锁定时
// 生效；所以 padding_override 必须和内边距放在一起。
struct BoxMetrics
{
    Edges padding{};
    bool padding_override = false;
    Edges margin{};
};
} // namespace xel::ui
