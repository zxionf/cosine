# 20261025-m3-5-table

<!-- 为什么改、边界与不属于本次的范围 -->

## Table

- `TableColumn`：`title` / `width` / `align`（Left/Center/Right）；`width <= 0` 表示弹性列，
  剩余宽度在弹性列间均分。
- `Table`：固定表头 + 可滚动正文。
  - `set_columns` / `set_rows` / `add_row` / `clear_rows` / `row_count` / `rows`
  - `set_selected(i)` / `selected()` / `set_on_select`
  - `set_row_height` / `set_header_height` / `set_striped`
  - `scroll_offset()`；滚轮按 1.5 行滚动
- 绘制：表头背景 + 底部分隔线；正文行支持选中高亮（focus_ring 半透明）、
  hover 高亮、斑马纹；列文本按对齐方式绘制并按列宽裁剪。
- 交互：点击行选中；`Up/Down/Home/End` 移动选中并自动滚动保持可见；
  表头点击不改变选中；`focusable()` 参与 Tab 遍历。
- 列宽按「去掉滚动条后的正文宽度」分配，避免右对齐的最后一列被滚动条裁掉。

## 同时修复

- `graphics::Rectangle` 在预乘 alpha 管线（One / OneMinusSrcAlpha）下没有把 rgb 乘以 alpha，
  半透明矩形（选中高亮、斑马纹、文字选区、遮罩）显得过亮。
  现在构造顶点时统一预乘，视觉正确。

## 边界

- 不支持列宽拖拽、列排序、多选、单元格编辑、横向滚动。
- TreeView 留到下一步。
