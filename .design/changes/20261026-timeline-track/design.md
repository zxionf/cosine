# 20261026-timeline-track

<!-- 为什么改、边界与不属于本次的范围 -->

## M4：时间轴、轨道与关键帧

### 为什么

属性系统解决了「怎么读写」，还差「按时间采样」：轨道存关键帧，播放时按当前时间插值并写回场景。

### 设计

`src/anim/timeline.hpp` / `timeline.cpp`（并入 `cosine_anim`）：

- `Keyframe { time, value, easing, custom }`：缓动取**区间终点帧**的曲线。
- `Track { node, property, keys }`：
  - `add_key()` 按时间插入（`upper_bound`），乱序添加也会排序；
  - `sample(time)`：两端外推取端点；单帧恒定；相邻帧之间 `local = (t - a) / (b - a)`，
    `ease(b.easing, local, b.custom)` 后插值。
- `lerp_property(a, b, t)`：float / vec2 / vec3 / vec4 用 `glm::mix`；
  bool / string / uint32 为**阶跃**（`t < 1` 取 a，到终点才切换）。
- `Timeline`：
  - `duration` / `fps` / `loop` / `time_scale`；
  - `play` / `pause` / `seek` / `advance(dt)`；
  - `seek`：循环模式取模到 `[0, duration)`，非循环 clamp 到 `[0, duration]`；
  - `advance`：非循环到末尾停在 `duration` 并自动暂停；
  - `add_track(node, prop)`；`apply(scene)` / `apply(scene, time)` 把采样结果写回场景。

### 验证

新建 `test/test_anim_timeline.cpp` 并注册：

- `lerp_property`：float / vec2 / vec4 插值；bool / string 阶跃。
- `Track::sample`：空轨道默认值、单帧恒定、两帧线性中点、两端外推、乱序添加后有序。
- 缓动：同一时间，`EaseIn` 的中点值小于 `Linear`。
- `Timeline::advance`：循环取模、非循环停在末尾并暂停、`time_scale` 生效。
- `Timeline::seek` + `apply`：写回 Scene 的值正确，且**同一时间重复 apply 结果一致**（确定性）。
- `apply(scene, time)` 与 `seek + apply` 等价。

### 边界（本次不做）

- 不做关键帧的增删改查 UI、不做轨道排序（M6 编辑器）。
- 不做「时间轴事件 / 回调」（M4 后续的 events）。
- 不做序列化（结构已就位：id 稳定、属性 id 稳定）。
