# 20261025-m3-5-dialog 任务清单

## Dialog / Modal

- [x] T01 `Dialog : Panel`：标题 + 正文 Vbox + 按钮 Hbox，`set_message` / `add` / `add_action`
- [x] T02 `DialogHost`：全屏遮罩、居中卡片、吞点击与滚动、Esc 冒泡关闭
- [x] T03 关闭语义：`remove_overlay` 延迟销毁，`on_close` 只回调一次，`dismissable` 开关
- [x] T04 `show_dialog(root, dialog, on_close)` 便捷入口 + `preferred_focus`
- [x] T05 新增 `test_ui_dialog`（遮罩点击、按钮回调、Esc、不可关闭、重复 close）
- [x] T06 gallery Overlays 分区接入（Open dialog + 状态回显 + COSINE_GALLERY_DIALOG）
- [x] T07 全量回归（23/23 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成并关闭；决策 #45
