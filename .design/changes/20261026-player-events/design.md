# 20261026-player-events

<!-- 为什么改、边界与不属于本次的范围 -->

## M4：播放器与时间点事件

### 为什么

`Timeline` 与 `Scene` 的绑定（advance + apply）此前散落在 demo 里，且缺少「时间点回调」。
M4 的验收要求「暂停、拖动、逐帧确定性 render」，还需要一个统一入口，
并给 M5（脚本）留下事件挂载点。

### 设计

`src/anim/player.hpp` / `player.cpp`（并入 `cosine_anim`）：

- `Player(scene::Scene&, Timeline&)`：持有引用，不拥有。
- `tick(dt)`：仅在播放中推进；推进后**先触发跨过的事件，再 apply**。
- `seek(time)`：确定性跳转 → `timeline_.seek + apply`，**不触发事件**。
- `apply()`：按当前时间重新采样（外部改了轨道后强制刷新）。
- `play/pause/toggle/playing/time`。
- `add_event(time, callback)` / `clear_events()` / `event_count()`：
  - 事件用 `(before, after]` 半开区间判定，起点不重复触发；
  - 循环回绕（`after < before`）时，跨 0 的事件也会触发，于是下一轮重新生效；
  - 一次 tick 跨多个事件会全部触发。

### demo

`ui_anim_demo` 改用 `Player`：

- `Demo` 先存在，再在 `build()` 里 `make_unique<Player>(demo.scene, demo.timeline)`（引用稳定）；
- `update(dt)` → `player->tick(dt)`；Pause/Play → `player->toggle()`；Reset → `player->seek(0)`。

### 验证

新建 `test/test_anim_player.cpp` 并注册：

- tick 推进并写回场景；事件只在跨过时触发一次。
- 暂停时 tick 不推进；`seek` 改变时间并 apply，但**不触发事件**。
- 循环回绕后事件重新生效（第二个周期再次触发）。
- 一次 tick 跨过多个事件，全部触发。
- `clear_events` 后不再触发。

### 边界（本次不做）

- 不做「进入/离开区间」的成对事件（当前是单点回调）。
- 不做事件排序 / 优先级；声明顺序即触发顺序。
- 不做事件在 `seek` 时的补发（确定性跳转只同步状态）。
