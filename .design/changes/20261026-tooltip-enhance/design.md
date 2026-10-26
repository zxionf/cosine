# 20261026-tooltip-enhance

<!-- 为什么改、边界与不属于本次的范围 -->

## Tooltip

- 延迟显示：`set_delay(seconds)`（默认 0.35s）；悬停累计到阈值才显示，离开立即隐藏并清零计时。
- 跟随光标：显示后每帧按光标位置更新，默认偏移 `(16, 18)`。
- 边界翻转：给定可用区域 `bounds`（通常是 root 的 `global_bounds()`），右下溢出时翻到左上；
  翻转后仍把位置夹在 bounds 内，避免被窗口裁切。
- 驱动接口：`update_hover(dt, over_target, cursor, bounds)`；
  保留 `show_at` / `hide` 用于立即显示与手动控制。
- 位置写的是相对 root 的局部坐标（`set_pos`），由 `UIRoot::layout` 转成全局坐标。

## 边界

- Tooltip 仍是无命中、无锚点的 overlay；目标判定由调用方（gallery）负责。
- gallery 用 `TipTarget` 列表维护「控件 + 文本」：watermark、Inputs 按钮、Overlays 按钮与靠近右边缘的宽色条；
  每个目标独立提示，边界色条用来直观验证翻转。
- 不支持富文本、图标、指向箭头；淡入淡出留到 M4 动画内核评估。
