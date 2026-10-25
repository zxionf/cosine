# 20261025-gallery-scroll-sections handover

## 交付

- 新控件 `src/ui/scroll_panel.hpp`：`ScrollPanel : Panel`，内部 ScrollView + Vbox，
  `add()` 进入滚动内容区，面板边框/背景与 padding 复用 Panel。
- `examples/ui_gallery`：所有分区（6 页）改用 `ScrollPanel`，内容超出可视区时可滚动。
- `test/test_ui_gallery.cpp`：断言每个分区都有 ScrollView、Containers 分区内容高度大于视口、滚轮可滚动。

## 关键结论

- `ScrollView::measure()` 返回视口尺寸，因此 ScrollPanel 直接按内边距矩形 arrange 即可填满。
- 分区标题随内容滚动（如需固定表头，后续给 ScrollPanel 加 header 槽）。

## 回归

22/22 ctest；ASAN 22/22；lavapipe 冒烟通过。
