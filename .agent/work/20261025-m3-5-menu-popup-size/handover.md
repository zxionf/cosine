# 20261025-m3-5-menu-popup-size handover

## 问题

菜单悬停切换后内容更新了，但弹层宽高仍是第一个菜单的尺寸。

## 根因

`MenuPopup` 构造时 `set_min_size/set_max_size({width, height})` 固化了初始尺寸；
`UIRoot::layout_child()` 每帧执行 `clamp_size(child.measure(...))`，
新的尺寸被夹回旧的 min/max，`arrange()` 因此永远拿到初始值。

## 修法

- 构造只写 `local_ = {{0,0},{width,height}}`，不设 min/max；
- `arrange()` 直接写 `local_/global_`；
- `measure()` 返回 `local_.size`。

## 验证

- File（115px 高）→ 4 项菜单：高度变化
- 长标签菜单：宽度 > 160px
- 24/24 ctest；ASAN 24/24；lavapipe 冒烟通过
- 截图：File 弹层明显高于 Edit 弹层

## 经验

「尺寸由外部计算、直接 arrange」的控件不要再用 min/max_size 固化尺寸，
否则 `UIRoot::layout_child()` 的 `clamp_size` 会把它夹回旧值。
