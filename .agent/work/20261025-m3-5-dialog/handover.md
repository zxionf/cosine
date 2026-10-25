# 20261025-m3-5-dialog handover

## 交付

- `src/ui/dialog.hpp`：`Dialog`（卡片：标题/正文/按钮）、`DialogHost`（遮罩 + 居中 + 吞事件）、`show_dialog()`
- `test/test_ui_dialog.cpp`：遮罩点击、按钮回调、Esc、`dismissable=false`、重复 close 只回调一次
- `examples/ui_gallery`：Overlays 分区「Open dialog」按钮，`COSINE_GALLERY_DIALOG=1` 启动即打开

## 关键结论

- Esc 依赖「焦点控件 → 父链」冒泡；无按钮对话框需让宿主自身可聚焦（`preferred_focus()` 已兜底）。
- 关闭必须走 `UIRoot::remove_overlay`（延迟销毁），事件派发期间不能析构自身。
- 遮罩必须吞掉 scroll/click，否则背景会滚动或误触。

## 回归

23/23 ctest；ASAN 23/23；lavapipe 冒烟通过。

## 后续

M3.5 继续 Menu/MenuBar、Table/TreeView、Tooltip 延迟/翻转、Grid/Wrap 布局。
