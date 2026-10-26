# 20261026-drag-feedback

<!-- 为什么改、边界与不属于本次的范围 -->

## 拖拽反馈

### 现状

- `Slider` 拖动时只有值在变：手柄与轨道颜色几乎不变，也看不到当前数值。
- `Splitter` 拖动时手柄样式与 hover 相同，没有"正在拖拽"的视觉；`ratio` 固定 clamp 在 `[0.1, 0.9]`，不考虑子项自己的 `min_size`，子项可能被压到 0 宽。

### 设计

**Slider**

- 拖动中（`dragging_`）：
  - 手柄外圈一条 `accent` 描边环（`append_rounded_ring`，厚度 2，半径 = 手柄半径 + 1）。
    复用焦点环的几何，但**不依赖** `FocusPolicy`（拖拽反馈不是焦点，键盘策略为 Never 时也应显示）。
  - 手柄上方显示数值气泡：圆角矩形 + 居中文本。
    宽度 = 文本宽 + `padding`，高度 = 行高 + `padding * 0.5`；
    位置 = 手柄中心上方 10px，水平中心对齐手柄，并 clamp 到控件 bounds 内（避免被父级裁剪）。
  - 文本格式：`step_ >= 1` 用 `{:.0f}`，否则 `{:.2f}`；没有 `font_atlas` 时跳过气泡。
- hover（未拖动）：维持现有的手柄混入 `accent` 15%。
- 边界：`value_` 已有 clamp；气泡只反映当前值。
- 副作用：拖动期间顶点数增加（气泡 + 环），松开后恢复原状。

**Splitter**

- hover：维持现有 `Control` hover 填充 + 主题圆角。
- 拖动中（`dragging_`）：手柄填充改为 `accent` 28% 半透明，并叠加 `accent` 描边环（厚 1），明确"正在拖拽"。
- 边界约束：`set_ratio` 保留 `[0.1, 0.9]` 兜底，并按子项 `min_size` 推导实际上下限：

```text
usable = 主轴尺寸 - handle_size
lower  = clamp(first_min / usable, 0, 0.9)
upper  = clamp(1 - second_min / usable, lower, 1)
ratio  = clamp(ratio, max(0.1 兜底…仅当 min_size 为 0), ...)
```

取主轴分量：`Horizontal` 用 `min_size().x`，`Vertical` 用 `min_size().y`。
`arrange` 时也 clamp 一次，保证容器尺寸变化后不越界。

**gallery**

- `Inputs` 分区 Slider 标题补「drag shows value bubble」。
- `Containers` 分区 Splitter 子面板设 `min_size`，标题补「drag highlight, respects child min size」，用于人工验收约束效果。

### 验证

`test_ui_drag_feedback`（纯逻辑）：

- Slider：按下后 `append_to` 顶点数增加（气泡 + 环），松开后恢复；拖动到范围外时值仍被 clamp；`step_ >= 1` 与 `< 1` 两种气泡都不崩。
- Splitter：给子项设较大 `min_size` 后拖到极端，`ratio` 被限制在可满足 min_size 的区间；`dragging` 时顶点数增加；松开后恢复。
- 现有 32 个测试全部通过；ASAN 通过；gallery 冒烟 `validation=0`。

### 边界（本次不做）

- 不做鼠标光标形状切换（需要 window API，属应用层）。
- 不做拖拽起始阈值、双击复位、拖拽结束吸附。
- 气泡不做出现/消失动画（瞬切）。
- 不改 `Splitter::handle_size`（固定 6），不改 Slider 的轨道内缩布局。
