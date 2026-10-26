# 20261026-anim-demo

<!-- 为什么改、边界与不属于本次的范围 -->

## 动画演示 example + 绝对定位漂移修复

### 为什么

M4 的 `scene` 还没有渲染路径，但 `easing` 已经可用。需要一个能直观看到「缓动差异」的程序：
同一时间、五条曲线驱动的方块位置并列对比，同时验证现有 UI 控件承载动画的能力。

### example

`examples/ui_anim_demo/main.cpp`（target `ui_anim_demo`，链接 `cosine_app` + `cosine_anim`）：

- 舞台 `Panel` 内 5 条轨道：`Linear` / `EaseIn` / `EaseOut` / `EaseInOut` / 自定义 cubic-bezier，
  每条一个绝对定位的 `ColorRect`，位置 = `origin + ease(t) × span`。
- 控制：`Pause` / `Play` 切换、`Reset` 归零；`Progress` 显示当前归一化时间，`Label` 显示秒数。
- 时间用真实 `frame_time_ms` 推进，周期 3s 循环。

### 顺带修复：绝对定位位置漂移

`absolute` 子项的 `x()/y()` 是 `local_.pos`，而 `arrange` 会把它改写成**全局坐标**；
容器下一帧又用 `inner_x + child->x()` 计算，导致每帧累加父偏移，方块一路飘出屏幕。

修复：

- `Widget` 增加 `absolute_pos_`（相对父内容区的偏移）与 `set_absolute_pos()` / `absolute_pos()`；
  `set_absolute(true)` 时以当前 `local_.pos` 作为初始偏移（兼容旧用法）。
- `Vbox` / `Hbox` / `Grid` / `Wrap` 的绝对定位分支改用 `absolute_pos()`，不再读 `x()/y()`。
- `ColorRect` 增加 `set_color()`（示例按曲线着色，也便于后续动画）。

### 验证

- `test_ui_absolute_zindex` 新增用例：连续 4 次 `arrange` 后绝对定位子项位置不漂移。
- `ctest` 43/43（build 与 ASAN）。
- `ui_anim_demo` 冒烟 `validation=0`；截图确认 5 条轨道、进度条与时间标签。
- gallery 冒烟未受影响。

### 边界

- 不做 scene 渲染（M4 后续）；示例只用现有 UI 控件。
- 不做时间轴拖动 seek（先用 Pause/Reset；seek 属于 M4 的 player）。
