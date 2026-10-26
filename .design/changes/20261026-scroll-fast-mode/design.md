# 20261026-scroll-fast-mode

<!-- 为什么改、边界与不属于本次的范围 -->

## 第三种滚动模式：Fast（停止输入后快速收尾）

### 为什么

`Smooth` 的时间常数是 0.12s，偏柔和：滚轮停止后仍有明显「滑行尾巴」。
需要一种「一停就定」的手感，同时保留平滑过渡（而不是 `Line` 的瞬时跳变）。

### 设计

`ScrollMode` 增加 `Fast`：

- 基础时间常数 = `max(0.03, smoothing × 0.4)`（默认约 0.048s，Smooth 的 40%）；
- 吸附阈值 0.5px（Smooth 是 0.25px）；
- **停止输入超过 0.1s 后时间常数再减半**（约 0.024s），视觉上快速定住。

`idle_` 由控件的 `on_scroll` 清零、`on_update` 累加，不需要外部时间源，
也不依赖 `UISystem::elapsed_`。

### 接入

- `ScrollView` / `TreeView` / `Table` 三个滚动容器都支持 `Fast`。
- gallery Style 分区增加 `Fast` 按钮，与 `Smooth` / `Line` 并列；
  `COSINE_GALLERY_SCROLL_MODE=fast` 同样可选。

### 验证

- `test_ui_smooth_scroll` 新增用例：同样 0.096s，`Fast` 的位移大于 `Smooth`；
  再 0.64s 后 `Fast` 精确吸附到目标。
- `ctest` 41/41（build 与 ASAN）。
- gallery 冒烟与截图（Style 分区三个按钮）。

### 边界

- 不引入初速度 / 惯性模型；三种模式仍是「目标跟随」。
- 不改变 `Smooth` 与 `Line` 的既有参数与语义。
