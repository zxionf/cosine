# 20261026-grid-layout 交接

## 结果

- 新增 `Grid` 布局容器：`src/ui/grid.hpp`、`src/ui/grid.cpp`，已在 `src/CMakeLists.txt` 注册。
- 列宽支持等分与「固定 + 弹性权重」混合；行高按内容取最大；`set_cell_align` 控制格内对齐
  （默认 `Align::h_stretch()`）。
- gallery `Layout` 分区新增 2 列表单（标签列 120 + 内容列弹性）与 3 列按钮矩阵。
- 新增 `test/test_ui_grid.cpp`（10 个用例），注册为 `ui_grid`。

## 验证

- `ctest --test-dir build`：36/36；`ctest --test-dir build-asan`：36/36。
- gallery 冒烟 Layout 分区（默认 / `COSINE_GALLERY_CLASSIC=1` / `COSINE_GALLERY_RADIUS=2`）：
  `validation=0`。
- `sele lint` 四项 ok；`sele close` 已把「Grid 布局容器」折叠进 SPEC。

## 实现要点

- `set_column_widths({...})`：正值 = 固定像素，非正值 = 弹性权重（`-1` 即 1fr）；
  `set_columns(n)` 等价于 n 个 `-1`；空/非法输入回退 1 列。
- 固定列总和超过可用宽时等比压缩，弹性列归零，避免负宽。
- `measure` 在 `constraints.x >= 1e8`（无界）时，弹性列取该列子项自然宽的最大值。
- 行高 = 该行子项内容高（含上下 margin）的最大值，紧凑排布；不把 slot 多余高度均分给各行。
- 单元格内按 `align_self()`（缺省 `cell_align`）摆放，`stretch` 时填满；`flex_grow` 被忽略。
- 本版不做跨列/跨行、固定行高、自动重排。

## 后续

- M3.5 布局补充下一项：`Wrap` / Flow 布局（可复用本变更的「列宽解析 + 位置数组」思路）。
- 之后是百分比尺寸与 aspect ratio、绝对定位 / z-index。
- 遗留（非本变更）：面板/弹层出现消失过渡推迟到 UI 打磨收尾；缓动曲线可配置推迟到 M8。
