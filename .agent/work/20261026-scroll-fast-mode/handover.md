# 20261026-scroll-fast-mode 交接

## 结果

- `ScrollMode` 增加 `Fast`：`tau = max(0.03, smoothing × 0.4)`，吸附 0.5px，
  **停止输入 0.1s 后 `tau` 再减半**。
- `ScrollView` / `TreeView` / `Table` 三个滚动容器都支持；`idle_` 由 `on_scroll` 清零、
  `on_update` 累加（不依赖外部时钟）。
- gallery Style 分区：`Smooth` / `Fast` / `Line` 三个按钮 + 状态标签；
  `COSINE_GALLERY_SCROLL_MODE=fast` 也可选。

## 验证

- `test_ui_smooth_scroll` 新增用例：同样 0.096s，Fast 位移大于 Smooth；再 0.64s 精确吸附。
- `ctest` 41/41（build 与 ASAN）。
- 冒烟：Style、Style(fast)、Containers(fast) 三组 `validation=0`。
- `sele close` 为 `appended`，SPEC 的 `## TreeView` 与「滚动目标保持」均未被覆盖。

## 三种模式对照

| 模式 | 行为 | 时间常数 | 吸附 |
|---|---|---|---|
| Smooth | 柔和跟随 | 0.12s | 0.25px |
| Fast | 一停就定 | 0.048s（停 0.1s 后 0.024s） | 0.5px |
| Line | 立即到位 | — | — |

## 后续

- M4 按 easing → scene-node → property-system 启动。
