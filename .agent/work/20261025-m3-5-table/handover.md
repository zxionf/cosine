# 20261025-m3-5-table handover

## 交付

- `src/ui/table.hpp`：`TableColumn` + `Table`（表头、行、选中、hover、斑马纹、滚动条、键盘）
- `test/test_ui_table.cpp`：点击选中、表头不选中、键盘导航、滚轮、自动滚动、clear/add
- `examples/ui_gallery`：Containers 分区 14 行资源表 + 选中回显

## 关键结论

- 列宽必须按「正文宽度（扣掉滚动条）」分配，否则右对齐列被裁。
- `Rectangle` 原先没做预乘 alpha，半透明色在 `One / OneMinusSrcAlpha` 下会偏亮；
  已在 `Rectangle::append_to` 统一预乘，选区/遮罩/斑马纹/选中高亮一起变正确。
- `set_selected` 在布局前（视口高度 0）不应滚动，否则初始 offset 不为 0。

## 回归

25/25 ctest；ASAN 25/25；lavapipe 冒烟通过。

## 后续

TreeView（层级 + 展开/折叠）、列宽拖拽、多选、横向滚动。
