# 20261026-anim-showcase

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.5：动画能力全面展示

### 为什么

M4 的内核已齐（scene / property / timeline / easing / player / SceneView），但 `ui_anim_demo`
只展示了「五条缓动的 position 轨道」。需要一个展厅，把 M4 的能力逐项摆出来，
既作为 M4.5 的验收，也作为 M5 之前的对外演示。

### 展厅结构（`examples/ui_anim_showcase`）

单个 `SceneView`（720×500）内分三个展区，全部由 `Scene + Timeline + Player` 驱动：

- **A. 缓动**：五条 `position` 轨道（Linear / EaseIn / EaseOut / EaseInOut / 自定义
  cubic-bezier），同一 6s 周期、同一区间，直观对比曲线差异。
- **B. 层级**：一个 `Group` 做 `scale` 脉冲（1.0 → 1.4 → 1.0），三个子方块以相对坐标跟随，
  验证父子变换累积。
- **C. 属性**：
  - `position`（往返）+ `scale`（横向拉伸）+ `color`（红 ↔ 蓝）三条轨道驱动同一个方块；
  - `text` 阶跃轨道：`phase: 0/1/2/3`；
  - `visible` 阶跃轨道：每秒切换一次，带说明文字。

### 控制与事件

- `Pause/Play`（`Player::toggle`）、`Back 1s` / `Fwd 1s` / `Reset`（`Player::seek`）；
- `Speed 1x` 在 0.5x / 1x / 2x 之间轮换（`Timeline::set_time_scale`）；
- `Loop: On` 切换循环（`Timeline::set_loop`）；
- `Progress` 与时间标签读 `Player::time()`；
- 时间点事件：2.0s / 4.0s 更新底部事件标签（`Player::add_event`）。

### 构建与冒烟

- `examples/CMakeLists.txt` 新增 `ui_anim_showcase`（链接 `cosine_app` + `cosine_anim`，
  带 sanitizer）。
- `COSINE_EXIT_AFTER=2 ./ui_anim_showcase`：exit=0、`validation` 计数 0。

### 验证

- 截图确认三个展区、控制行、进度与时间标签、事件标签均正常。
- `ctest` 47/47（既有测试不受影响）。

### 边界（本次不做）

- 不做 `rotation` 动画（`SceneView` 暂不支持旋转，M6/M7 再议）。
- 不做 `Image` 真实纹理动画（M6 资源管线）。
- 不做时间轴拖拽滑块（用 Back/Fwd/Reset 代替；滑块与 `set_value` 回调解耦放到 M6 编辑器）。
