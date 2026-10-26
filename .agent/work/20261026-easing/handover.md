# 20261026-easing 交接

## 结果

- 新建 `src/anim/` 与静态库 `cosine_anim`（只依赖 glm，不依赖 UI）。
- `anim/easing.hpp`：`Easing{Linear, EaseIn, EaseOut, EaseInOut, CubicBezier}`、
  `CubicBezier{x1,y1,x2,y2}::operator()(x)`、`ease(Easing, t)` / `ease(Easing, t, custom)`。
- `anim/easing.cpp`：二分求逆（24 次，精度约 6e-8），`x1/x2` clamp 到 `[0,1]`，
  输入 `t` clamp 到 `[0,1]`（同时挡住 NaN）。
- 预置：`kEaseIn(0.42,0,1,1)`、`kEaseOut(0,0,0.58,1)`、`kEaseInOut(0.42,0,0.58,1)`。
- 新增 `test/test_anim_easing.cpp`（7 个用例），注册为 `anim_easing`。

## 验证

- `ctest` 42/42（build 与 ASAN）。
- `sele close` 为 `appended`，SPEC 已有「M4 起手：缓动库」小节，
  且「第三种滚动模式」等既有小节未被覆盖。

## 下一步（M4 顺序）

1. **scene-node**：`scene::Node`（稳定 id、name、transform、visible、children）+
   Group / Shape / Text / Image / Camera 的类型方案。
2. `property-system`：属性注册 + 类型擦除读写。
3. `timeline` / `track` / `keyframe`（用本变更的 `ease()`）。
4. `player`（确定性 seek）、`events`、序列化 ID。

## 设计提醒

- 缓动是纯函数，不要在 `ease` 里做 clamp 之外的状态处理。
- 自定义控制点的 `x1/x2` 必须 clamp，否则 `x(s)` 非单调、二分求逆会给出错误结果。
