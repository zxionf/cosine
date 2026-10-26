# 20261026-grid-layout

<!-- 为什么改、边界与不属于本次的范围 -->

## Grid 布局容器

### 为什么

现有容器只有 `Vbox`/`Hbox` 两种一维排布。M6 时间轴编辑器的检查器面板是典型的
「左标签 / 右控件」两列表单，用一维容器拼需要嵌套多层 Hbox 且列宽无法对齐；
工具栏、属性网格也需要规则的多列布局。`Grid` 用一次排布给出对齐的列。

### API

`class Grid : public Widget`，与 `Vbox`/`Hbox` 同层，放 `src/ui/grid.hpp` + `grid.cpp`。

列宽规格二选一，互斥：

- `set_columns(int count)`：`count` 等分（每列权重 1），非法值（<=0）回退 1 列。
- `set_column_widths(std::vector<float> widths)`：逐列指定。
  - `> 0`：固定像素宽；
  - `<= 0`：弹性列，按 `|值|` 作权重分享剩余空间（`-1` 即常见的 `1fr`）；
  - 全部非正时按权重分整宽；空列表回退 1 列。

其余设置：

- `set_spacing(float all)` / `set_spacing(float x, float y)`：列间距、行间距。
- `set_cell_align(Align)`：单元格内对齐，默认 `Align::h_stretch()`
  （横向填满单元格、纵向靠上），表单场景最常用。子项可用 `set_align()` 单独覆盖。
- padding / margin / min_size / max_size / clamp_size 沿用 `Widget`。

### 排布规则

- 行数由子项数推导：`rows = ceil(children / cols)`，不单独设置；子项增多自动加行。
- 第 `i` 个子项落在 `row = i / cols`、`col = i % cols`。
- 行高 = 该行所有子项 measure 高度（含上下 margin）的最大值；行内各格等高。
- 列宽在 `arrange` 时按实际 slot 宽计算：先扣 padding 与间距，固定列取指定值，
  剩余宽度按弹性权重分配；固定列总和超过可用宽时按比例压缩，避免负宽。
- 单元格内的子项可用区域 = 格宽 − 水平 margin，格高 − 垂直 margin；
  按 `align_self()`（缺省用 `cell_align`）在可用区域内摆放，`stretch` 时填满。
- `measure(constraints)`：
  - 列宽确定规则同上；`constraints.x` 为无界（>= 1e8）时，弹性列取该列子项的自然宽最大值，
    使「内容自适应」可用；
  - 行高按列宽约束测量后取行内最大；总高 = 行高和 + 行间距 + padding；
  - 与 `Vbox` 一致做 `(constraints, measure)` 缓存。
- 忽略子项 `flex_grow()`：格大小由列宽/行高决定，不做纵向剩余空间分配（与 `Vbox` 语义不同）。
- 本版不做跨列（span）/ 跨行、不做固定行高、不做自动行列重排。

### 绘制与命中

- `hit_testable() == false`，无自身绘制（仅 debug 边框与子项）。
- `append_to` 只遍历可见子项，与 `Vbox` 一致。

### 验证

新建 `test/test_ui_grid.cpp` 并注册到 `test/CMakeLists.txt`：

- 等分：3 列 × 2 行，检查每格 x/宽与行 y/高、间距与 padding。
- 固定 + 弹性：`{80, -1}`，检查第一列 80、第二列吃满剩余。
- 权重：`{-2, -1}` 按 2:1 分配。
- 无界约束下 measure：弹性列取自然宽最大值。
- 行高取行内最大，总高 = 行高和 + 间距 + padding。
- 默认 `h_stretch` 横向填满；`set_align(center)` 覆盖后按内容居中。
- 子项 margin 生效；空 Grid / 非法列数不崩、回退 1 列。

gallery `Layout` 分区增加示例：

- 2 列表单（标签 + 输入框，标签列固定 120，内容列弹性）；
- 3 列按钮矩阵（等分），用于人工验收对齐与间距。

### 边界（本次不做）

- 跨列 / 跨行、固定行高、自动重排（`grid-auto-flow: dense` 之类）。
- 百分比尺寸与 aspect ratio 属于后续「百分比尺寸」变更。
- 不做 Grid 的线条/背景绘制（只做布局，视觉由子项与父容器提供）。
