#pragma once

namespace xel::ui
{
// 滚动步进模式：
// - Smooth：滚轮只改变目标偏移，实际偏移按时间常数逼近（丝滑）；
// - Fast：同 Smooth，但时间常数更小，且停止输入后进一步加速收尾（一停就定）；
// - Line：滚轮直接跳到目标（离散按行/按步长，保持旧观感）。
enum class ScrollMode
{
    Smooth,
    Fast,
    Line,
};
} // namespace xel::ui
