# 20261026-percent-aspect

<!-- 为什么改、边界与不属于本次的范围 -->

## 百分比尺寸与宽高比

### 为什么

M6 时间轴编辑器的预览卡需要固定 16:9；属性面板需要「占一半宽」「撑满高」这类相对尺寸。
现在所有控件只能用固定像素，容器尺寸变化后子项不跟随，只能在外层手工换算。
把「相对尺寸」下沉到 `Widget` 与容器的测量契约里，一处声明、所有容器生效。

### API（Widget 层）

- `set_size_percent(glm::vec2 pct)`：每轴占父级**可用空间**的比例。
  `> 0` 启用，`<= 0` 不启用（默认两轴都不启用）。
- `set_aspect_ratio(float ratio)`：宽 / 高。`> 0` 启用，默认关闭。
- 两者可组合；`min_size` / `max_size` 仍然生效（最后 clamp）。

### 解析规则（`Widget::measure_child`）

容器测量子项时统一走 `static glm::vec2 Widget::measure_child(Widget& child, glm::vec2 constraints)`：

1. **百分比**：`constraints` 该轴有限（`< 1e8`）时，把传给子项的约束换成
   `constraints × pct`；约束无界时百分比不生效（退回子项自然尺寸）。
2. 子项按调整后的约束 `measure`；启用百分比的轴直接用调整后的约束值作为尺寸。
3. **宽高比**：
   - 指定了百分比高、没指定百分比宽 → `宽 = 高 × ratio`；
   - 其余情况 → `高 = 宽 / ratio`（优先宽）。
4. 最后 `clamp_size` 到 `min_size` / `max_size`。

百分比与容器对齐的关系：百分比先确定尺寸，`stretch` 不再覆盖它
（即 `set_size_percent({0.5, 0})` + `h_stretch` 时宽仍是一半）。

### 接入范围

- `Vbox` / `Hbox` / `Grid` / `Wrap` 的子项测量与排布统一改用 `measure_child`，
  替换原来的 `child->measure(...)` + `child->clamp_size(...)` 组合。
- `ScrollView` / `TabView` / `Dialog` 测量的是「内容自然尺寸」（内容应撑满或不受限），
  语义不同，本次不改；它们内部的 Vbox/Grid 等仍会正常解析子项百分比。

### 验证

新建 `test/test_ui_percent_aspect.cpp` 并注册：

- Vbox 中 `set_size_percent({0.5, 0})` → 宽 = 容器内容宽的一半。
- 百分比与 `stretch` 同时存在时，百分比优先。
- 无界约束下百分比不生效（退回自然尺寸）。
- `aspect_ratio` 由宽推高；百分比高 + ratio 时由高推宽。
- Grid 单元格内的百分比相对格宽；Wrap 内相对内容宽。
- `min_size` / `max_size` 仍生效。
- 未启用时行为与迁移前一致（现有 37 个测试全绿）。

gallery `Layout` 分区：
- 16:9 预览卡（宽 40%）；
- 50% 宽 + 100% 高的色块，验证相对尺寸随容器变化。

### 边界（本次不做）

- 不做相对兄弟节点或相对内容的百分比（CSS 的 min-content / auto）。
- 不做百分比 margin / padding。
- 不做绝对定位与 z-index（下一项）。
