# 20261025-m3-5-tab-view handover

## 交付

- `src/ui/tab_view.hpp`：`TabView`（TabBar + 页面容器，children 复用、可见性隔离、页面填满内容区）
- `test/test_ui_tab_view.cpp`：可见性、点击/键盘切换、clamp、set_page、回调
- `examples/ui_gallery`：Containers 分区接入三页 TabView + 状态回显

## 关键结论

- 页面容器把 TabBar 放在 `children_[0]`、页面随后，可直接复用 UISystem 的命中/焦点遍历。
- 切换只翻转可见性，非当前页自然不参与命中与 Tab 遍历。
- 页面默认填满内容区，避免页面内容按自然宽度居中。

## 回归

22/22 ctest；ASAN 22/22；lavapipe 冒烟通过。

## 后续

M3.5 继续 Dialog/Modal、Menu/MenuBar、Table/TreeView；gallery 各分区需要滚动容器。
