# 20261026-scroll-wheel-fix

<!-- 为什么改、边界与不属于本次的范围 -->

## Table / TreeView 无法滚动；gallery 增加滚动模式切换

### 现象

Table 与 TreeView 的滚轮滚动完全无效，而 ScrollView 正常。
gallery 缺少 Smooth / Line 的运行时切换（只能靠环境变量）。

### 根因

`Table::arrange` / `TreeView::arrange` 里写的是：

```cpp
scroll_ = clamp_scroll(scroll_);
scroll_target_ = scroll_;   // ← 每帧把滚轮目标清零
```

layout 每帧都会跑（子控件 `arrange` 里的 `set_size` 触发 `mark_layout_dirty`），
于是滚轮刚设置的 `scroll_target_` 在下一帧布局时被清零，`on_update` 看不到差值，
位置永远停在 0。`ScrollView::arrange` 用的是 `target_ = clamp_offset(target_)`（保留目标），
所以只有它正常。

调试证据：dispatch 后 `table.target=126`，下一帧打印 `target=0`。

### 修复

- `Table::arrange` / `TreeView::arrange` 改为 `scroll_target_ = clamp_scroll(scroll_target_);`
  —— 保留目标，只在可滚范围变化（内容变短）时收紧。
- `set_scroll_value()` 继续用于「程序化跳转 / 键盘 / 结构变化（set_roots、collapse）」：
  立即设置位置并同步目标。

### gallery

- `UiGallery::set_scroll_mode(ScrollMode)`：统一切换所有分区 `ScrollPanel`、
  示例 `ScrollView`（`scroll_vertical` / `scroll_both`）、`Table`、`TreeView`。
- Style 分区新增 **Smooth / Line** 两个按钮 + `scroll: smooth|line` 状态标签。
- `COSINE_GALLERY_SCROLL_MODE=line|smooth` 改为走同一入口，保持行为一致。

### 验证

- 复现程序：`on_scroll` → `arrange` → `update`，`target` 保持 126、`offset` 推进到 29.5。
- `ctest` 41/41（build 与 ASAN）。
- gallery 冒烟：Style、Containers、Containers(line) 三组 `validation=0`。
- 截图：Style 分区可见 Smooth / Line 按钮与 `scroll: smooth` 状态。

### 边界

- 不改滚轮步长、平滑参数与模式语义（沿用 M3.6 的定义）。
- 不做「每个容器独立的模式 UI」——gallery 只提供一个全局切换入口。
