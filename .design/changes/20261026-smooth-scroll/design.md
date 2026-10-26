# 20261026-smooth-scroll

<!-- 为什么改、边界与不属于本次的范围 -->

## M3.6 平滑滚动与离散行滚动

### 为什么

`ScrollView` 的滚轮是「像素步进 + 立即生效」，`TreeView` / `Table` 是「行步进 + 立即生效」，
都缺少过渡：滚一格内容瞬移，长列表阅读时容易丢失位置感。需要两件事：

- **平滑滚动**：滚轮只改变目标偏移，实际偏移按时间常数指数逼近，帧率无关；
- **保留离散按行滚动**：需要「一格 = 一行」的精确步进时仍可切换。

### API

新增 `src/ui/scroll_mode.hpp`：`enum class ScrollMode { Smooth, Line }`。

`ScrollView`：

- `set_scroll_mode(ScrollMode)` / `scroll_mode()`，默认 `Smooth`；
- `set_scroll_smoothing(float seconds)` / `scroll_smoothing()`，默认 `0.12`，`0` = 立即到位；
- `set_scroll_speed(float pixels_per_step)`，默认 `40`（Smooth 模式每格像素）；
- `set_scroll_line_step(float pixels_per_line)`，默认 `40`（Line 模式每格像素）；
- `scroll_target()` 暴露目标值（测试与程序化查询）。

`TreeView` / `Table`：

- `set_scroll_mode(ScrollMode)` / `scroll_mode()`，默认 `Smooth`；
- `set_scroll_smoothing(float seconds)` / `scroll_smoothing()`，默认 `0.12`；
- 步进单位保持「行」：一格 = `row_height × 1.5`；模式只决定「立即 / 过渡」。

### 平滑算法

`offset += (target - offset) × (1 - exp(-dt / tau))`：

- 与帧率无关：同样的总时长，不同 dt 分割不影响收敛轨迹；
- 差值 < 0.25px 时吸附到目标，避免无限逼近；
- 全程 `clamp` 到合法范围（内容变短时目标同步收紧）。

`set_scroll_offset()` 是程序化跳转：同时设置 `offset` 与 `target`（立即生效）。
拖动滚动条、点击轨道翻页直接改 `offset` 并同步 `target`（跟手优先，不做过渡）。

### 性能

- 平滑更新只调用 `place_content()`（内容平移），**不** `mark_layout_dirty()`，
  避免每帧整树重排；
- `TreeView` / `Table` 的绘制按 `scroll_` 直接算行位置，同样不触发重排；
- `on_update(dt)` 由已有的 `UISystem::update_tree` 驱动，无需新定时器。

### 验证

新建 `test/test_ui_smooth_scroll.cpp` 并注册：

- Smooth：`on_scroll` 后 `scroll_offset()` 不变、`scroll_target()` 变化；`update(dt)` 后单调逼近；
- 连续 `update` 后精确等于目标（吸附）；
- 帧率无关：同样总时长、不同 dt 分割，最终都吸附到同一目标；
- Line：`on_scroll` 后立即等于目标；
- clamp：目标不超出 `[0, max_scroll]`；
- `set_scroll_offset` 立即生效且同步 target；
- `Table`：Smooth 延迟、Line 立即（用 `set_rows` 造数据）。

gallery：新增 `COSINE_GALLERY_SCROLL_MODE=line` 切到离散行滚动，默认 `smooth`。

### 边界（本次不做）

- 不做惯性（flick）与速度衰减：只做「目标跟随」，不引入初速度模型。
- 不做滚动条拖动时的惯性/回弹/边缘橡皮筋。
- 不做触摸板亚像素 delta 的特殊处理（按同一比例缩放）。
