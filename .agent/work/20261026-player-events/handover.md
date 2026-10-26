# 20261026-player-events 交接

## 结果

- `src/anim/player.hpp/.cpp`（并入 `cosine_anim`）：
  - `Player(scene::Scene&, Timeline&)`：只持有引用；
  - `tick(dt)`：仅在播放中推进，**先触发跨过的事件，再 apply**；
  - `seek(time)`：确定性跳转 + apply，**不触发事件**；
  - `apply()` / `play()` / `pause()` / `toggle()` / `playing()` / `time()`；
  - `add_event(time, cb)` / `clear_events()` / `event_count()`。
- 事件语义：`(before, after]` 半开区间；循环回绕（`after < before`）时跨 0 的事件也触发，
  下一轮重新生效；一次 tick 跨多个事件全部触发。
- `ui_anim_demo` 改用 `Player`（Pause/Play → `toggle`，Reset → `seek(0)`）。
- 新增 `test/test_anim_player.cpp`（6 个用例），注册为 `anim_player`。

## 验证

- `ctest` 47/47（build 与 ASAN）。
- `COSINE_EXIT_AFTER=2 ./ui_anim_demo`：exit=0、`validation` 计数 0。
- `sele close` 为 `appended`（`DECISIONS #119`）。
- 阶段计划 M4 的 8 项全部勾选，标记「已完成，2026-10-26」。

## M4 交付清单

| 项 | 交付物 |
|---|---|
| 场景图 | `anim/scene.hpp`（`Scene`/`Node`/稳定 `NodeId`） |
| 属性系统 | `anim/property.hpp`（`PropertyValue` + `prop::*` + 注册表 + `Scene::get/set_property`） |
| 时间轴 | `anim/timeline.hpp`（`Timeline`） |
| 轨道/关键帧 | 同上的 `Track` / `Keyframe` |
| 缓动 | `anim/easing.hpp`（含 cubic-bezier 求逆） |
| 播放器 | `anim/player.hpp`（`Player`） |
| 事件 | `Player::add_event` |
| 渲染 | `ui/scene_view.hpp`（`SceneView`） |

## 下一步（M5）

把解释器改成可嵌入库（`xel_core`），提供宿主 API：创建/查找节点、设属性、
创建时间轴/轨道/关键帧、播放控制；示例用 XEL 脚本描述动画。

## 注意

- `Player` 只持有引用：`Scene` / `Timeline` 必须比它活得久。
- `seek` 不触发事件是有意为之（确定性跳转不应有副作用）。
- 事件是单点回调；如果需要「进入/离开区间」的成对语义，在回调里自行记录状态。
