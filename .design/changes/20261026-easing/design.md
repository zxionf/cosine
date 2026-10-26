# 20261026-easing

<!-- 为什么改、边界与不属于本次的范围 -->

## M4 起手：缓动库

### 为什么

M4 动画内核的第一块地基：关键帧之间需要「进度 → 进度」的曲线。
UI 的 `lerp_style` 用的固定 `ease_out_quad` 不足以覆盖 M4 的需求（cubic-bezier 是时间轴编辑器的
标准配置项），而且动画模块不应依赖 UI。先做一个独立、可纯逻辑测试的缓动库。

### 设计

新建 `src/anim/` 与静态库 `cosine_anim`（只依赖 glm，不依赖 UI），后续 `scene` / `timeline` /
`property` 都放进这个库。

`easing.hpp`：

```cpp
enum class Easing { Linear, EaseIn, EaseOut, EaseInOut, CubicBezier };

struct CubicBezier {   // 控制点 (0,0) (x1,y1) (x2,y2) (1,1)
    float x1, y1, x2, y2;
    float operator()(float x) const;   // 时间 → 进度
};

float ease(Easing, float t);
float ease(Easing, float t, const CubicBezier& custom);
```

- `x1` / `x2` 求值时 clamp 到 `[0,1]`：保证 `x(s)` 单调，才能求逆。
- 求值用二分法（24 次迭代，精度约 1e-7）；输入 `t` 先 clamp 到 `[0,1]`。
- 预置：`kEaseIn = (0.42, 0, 1, 1)`、`kEaseOut = (0, 0, 0.58, 1)`、
  `kEaseInOut = (0.42, 0, 0.58, 1)`（CSS 标准值）。
- `Easing::CubicBezier` 用传入的 `custom`；其它枚举走预置曲线。

### 验证

新建 `test/test_anim_easing.cpp` 并注册：

- 所有模式：`f(0) == 0`、`f(1) == 1`；输入越界被 clamp。
- `Linear` 恒等。
- 单调性：对每个模式，`t` 递增时结果不减（采样 100 点）。
- `EaseOut` 在中点大于 `EaseIn`（前者先快）。
- 自定义 cubic-bezier：`(0.25, 0.1, 0.25, 1)` 与预置曲线不同，且求解满足
  `|x(s) − x| < 1e-4`（通过 `operator()` 与已知点对比）。
- 极端控制点（`x1 = x2 = 0` / `= 1`）不产生 NaN。

### 边界（本次不做）

- 不做弹簧 / 物理缓动（M4 只需要曲线）。
- 不做缓动的编辑器 UI（M6）。
- 不替换 UI 里已有的 `ease_out_quad`（那是 UI 状态过渡的专用曲线）。
