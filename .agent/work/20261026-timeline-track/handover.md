# 20261026-timeline-track 交接

## 结果

- `src/anim/timeline.hpp/.cpp`（并入 `cosine_anim`）：
  - `Keyframe { time, value, easing, custom }`；
  - `Track`：`add_key` 有序插入；`sample(time)` 两端外推、单帧恒定、
    相邻帧按**区间终点帧**的 easing 插值；
  - `lerp_property`：float/vec2/vec3/vec4 用 `glm::mix`，bool/string/uint32 为阶跃；
  - `Timeline`：`duration` / `fps` / `loop` / `time_scale` / `play` / `pause` / `seek` /
    `advance` / `add_track` / `apply`。
- 新增 `test/test_anim_timeline.cpp`（7 个用例），注册为 `anim_timeline`。

## 验证

- `ctest` 45/45（build 与 ASAN）。
- `sele close` 为 `appended`（`DECISIONS #117`）。

## 关键语义

- `seek`：循环取模到 `[0, duration)`，非循环 clamp 到 `[0, duration]`；
- `advance`：非循环到末尾停在 `duration` 并自动暂停；
- 同一时间重复 `apply` 结果一致（确定性，M4 验收项）；
- `apply(scene, time)` 等价于 `seek(time) + apply(scene)`，但不会改变 `Timeline::time()`。

## 下一步（M4 收尾）

1. **player**：把 `Timeline::advance + apply` 接进主循环，提供逐帧确定性渲染与暂停/拖动。
2. **events**：时间点回调（进入/离开某时间、动画结束）。
3. **序列化前置**：节点/属性 id 已稳定，补 timeline 的读写接口（可放到 M6）。

## 注意

- 阶跃类型的区间终点用 `t < 1` 判断，意味着「到终点那一步才切换」。
- 缓动取终点帧：同一段区间若要不同缓动，需要在区间起点帧上调整（当前约定）。
