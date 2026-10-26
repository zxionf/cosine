# 20261026-wrap-layout

<!-- 为什么改、边界与不属于本次的范围 -->

## Wrap（流式换行）布局容器

### 为什么

`Grid` 要求列数固定；工具栏、标签云、卡片流这类「子项数量不定、宽度不一」的场景需要
「放不下就换行」。用 `Hbox` 嵌套无法表达，用 `Grid` 会在末尾留下空格。`Wrap` 一次排布
就能让子项按可用宽度自然折行。

### API

`class Wrap : public Widget`，放在 `src/ui/wrap.hpp` + `wrap.cpp`。

- `set_spacing(float all)` / `set_spacing(float dx, float dy)`：水平间距、行间距。
- `set_row_align(MainAlign)`：行内对齐，`Start`（默认）/ `Center` / `End` / `SpaceBetween`。
- `set_cross_align(Align)`：行内子项垂直对齐，默认 `Align::center_left()`
  （在行高内垂直居中）。子项可用 `set_align()` 单独覆盖。
- padding / margin / min_size / max_size / clamp_size 沿用 `Widget`。

### 排布规则

- 按子项顺序从左到右累加：当「当前子项含 margin 的宽」放不进剩余行宽，且该行已有子项时换行。
- 单个子项宽于整行时独占一行，按自身宽摆放（超出部分由父容器/滚动容器裁剪），x 不为负。
- 行高 = 行内子项高（含上下 margin）的最大值；行内子项按 `cross_align` 在行高内垂直摆放。
- 行内剩余宽按 `row_align` 分配：`Start` 靠左、`Center` 居中、`End` 靠右、
  `SpaceBetween` 均分到子项间隙（仅当行内至少两个子项）。
- `measure(constraints)`：
  - 有限宽（`< 1e8`）：按 `constraints.x - padding` 计算折行，总宽 = 最宽行的实际宽，
    总高 = 各行高之和 + 行间距 + padding；
  - 无界宽（`>= 1e8`）：不折行，全部排成一行；
  - 与 `Vbox` 一致做 `(constraints, measure)` 缓存。
- 忽略子项 `flex_grow()`：折行布局没有「纵向剩余空间分配」的概念。

### 绘制与命中

- `hit_testable() == false`，无自身绘制（仅 debug 边框与子项）。
- `append_to` 只遍历可见子项，与 `Vbox` 一致。

### 验证

新建 `test/test_ui_wrap.cpp` 并注册到 `test/CMakeLists.txt`：

- 有限宽折行：内容宽 100、5 个 30 宽子项、间距 5 → 第一行 3 个、第二行 2 个；检查 x/y 与总高。
- 无界宽：全部一行，总宽 = 5×30 + 4×5 = 170。
- `row_align`：`Center` / `End` / `SpaceBetween` 下的行内起始 x 与间隙。
- `cross_align`：行内高度不同的子项按 `center_left` 垂直居中。
- 单个子项宽于容器：独占一行、x 不为负。
- padding、margin、空容器、零/负间距不崩。
- measure 与 arrange 的结果一致（同一约束下总高相同）。

gallery `Layout` 分区：一排不同宽度的按钮/标签自动折行，用于人工验收。

### 边界（本次不做）

- 不做垂直方向的换列（column flow）、不做 `dense` 重排（不改变子项顺序）。
- 不做子项跨行。
- 百分比尺寸与 aspect ratio 属于后续「百分比尺寸」变更。
