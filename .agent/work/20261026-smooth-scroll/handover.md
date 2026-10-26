# 20261026-smooth-scroll 交接

## 结果

- 新增 `src/ui/scroll_mode.hpp`：`enum class ScrollMode { Smooth, Line }`。
- `ScrollView`：`target_` + `scroll_mode` / `scroll_smoothing` / `scroll_speed` / `scroll_line_step`
  / `scroll_target()`；`on_scroll` 只改目标，`on_update(dt)` 做帧率无关的指数逼近。
- `TreeView` / `Table`：同样的模式与平滑；步进单位不变（一格 = `row_height × 1.5`），
  Line 模式立即到位。新增 `set_scroll_value()` 统一「立即设置 + 同步目标」。
- 程序化跳转（`set_scroll_offset`）、拖动滚动条、轨道翻页直接改位置并同步目标。
- gallery：`COSINE_GALLERY_SCROLL_MODE=line|smooth`（默认 smooth）。
- 新增 `test/test_ui_smooth_scroll.cpp`（7 个用例），注册为 `ui_smooth_scroll`。

## 验证

- `ctest --test-dir build`：41/41；`ctest --test-dir build-asan`：41/41。
- gallery 冒烟：Inputs（smooth）、Inputs（line）、Layout（classic + line）三组 `validation=0`。
- `sele lint` 四项 ok。

## 关键实现点

- 平滑公式：`offset += (target - offset) × (1 - exp(-dt / tau))`，`tau` 默认 0.12s；
  差值 < 0.25px 吸附；全程 `clamp`。相同总时长、不同 dt 分割，结果一致。
- **平滑期间不调 `mark_layout_dirty()`**：ScrollView 只 `place_content()`，
  TreeView/Table 直接改 `scroll_`（绘制按它算行位置）。
- 默认 `Smooth`；需要「一格 = 一行立即到位」时设 `ScrollMode::Line`。
- 既有 4 个测试（`ui_layout` ×2、`ui_gallery`、`ui_scroll_snap` ×2、`ui_table`）显式声明 Line，
  因为它们断言的是「立即位移」。

## 后续

- M4 按建议顺序启动：`easing` → `scene-node` → `property-system`。
- 可选后续：惯性（flick）、边缘橡皮筋、触摸板亚像素 delta 处理。
