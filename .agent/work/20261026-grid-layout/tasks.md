# 20261026-grid-layout 任务清单

- [x] T01 `Grid` 类骨架：`grid.hpp` / `grid.cpp`，`add` / `children` / `append_to` / `layout`
- [x] T02 列宽规格：`set_columns` / `set_column_widths`（正=固定，非正=弹性权重）+ 非法输入回退
- [x] T03 `measure`：列宽解析（含无界约束取自然宽）、行高取行内最大、缓存与 padding/spacing
- [x] T04 `arrange`：固定列压缩保护、弹性权重分配、单元格对齐与 margin、`stretch` 填满
- [x] T05 `set_spacing(x, y)` / `set_cell_align`，默认 `h_stretch`
- [x] T06 `test/test_ui_grid.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T07 gallery `Layout` 分区：2 列表单 + 3 列按钮矩阵
- [x] T08 全量回归：ctest 36/36 + ASAN 36/36 + gallery 冒烟 3 组（validation=0）
- [x] T09 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- `Grid` 支持等分与「固定 + 弹性」列宽，行高按内容自动取最大，单元格对齐可配。
- 表单式布局（标签列固定、控件列填充）一行 `add` 即可完成，无需嵌套 Hbox。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验收。
