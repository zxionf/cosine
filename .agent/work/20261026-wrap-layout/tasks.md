# 20261026-wrap-layout 任务清单

- [x] T01 `Wrap` 类骨架：`wrap.hpp` / `wrap.cpp`，`add` / `children` / `append_to` / `layout`
- [x] T02 `measure`：有限宽按约束折行、无界宽单行、行高取行内最大、缓存与 padding/spacing
- [x] T03 `arrange`：折行、行内 `row_align` 分配剩余宽、`cross_align` 垂直对齐、margin
- [x] T04 `set_spacing(x, y)` / `set_row_align` / `set_cross_align`（默认 `center_left`）
- [x] T05 `test/test_ui_wrap.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T06 gallery `Layout` 分区：自动折行的标签/按钮流
- [x] T07 全量回归：ctest 37/37 + ASAN 37/37 + gallery 冒烟 3 组（validation=0）
- [x] T08 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- `Wrap` 按可用宽度自动折行，行内对齐与垂直对齐可配，measure 与 arrange 结果一致。
- 单个超宽子项不会把后续子项挤出可视区（独占一行）。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验收。
