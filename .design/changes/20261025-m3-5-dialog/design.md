# 20261025-m3-5-dialog

<!-- 为什么改、边界与不属于本次的范围 -->

## Dialog / Modal

- `Dialog : Panel`：标题 + 正文区（Vbox）+ 右下角操作按钮行（Hbox）。
  - `set_message()` 便捷正文（自动换行、宽度跟随卡片）。
  - `add()` 内容进入正文区；`add_action(label, handler, primary)` 追加按钮。
  - `preferred_focus()` 优先主按钮，否则第一个按钮。
  - `set_dismissable(false)` 禁止 Esc / 点击遮罩关闭。
- `DialogHost : Widget`：全屏 overlay，绘制半透明遮罩并居中对话框。
  - 点击卡片外 → 可关闭时 dismiss；所有点击都被吞掉，避免穿透。
  - 滚动事件被吞掉，背景不滚动。
  - Esc 通过按键冒泡（焦点控件 → 宿主）关闭。
  - `close()` 从 `UIRoot::remove_overlay` 移除自身（延迟销毁），`on_close` 只回调一次。
- `show_dialog(root, dialog, on_close)` 便捷入口，返回 `DialogHost*`。

## 布局与焦点

- 宿主按窗口尺寸铺满，卡片测量后居中，宽高被限制在窗口留 24px 边距内。
- 卡片 `cross_align = top_left`；按钮行 `h_stretch + main_align End`，标题左对齐、按钮右对齐。
- 打开时建议 `ui.set_focus(host->preferred_focus())`；无按钮时宿主自身可聚焦，保证 Esc 有效。

## 边界

- 不做窗口内拖拽、尺寸调整、嵌套模态栈、动画过渡。
- 不做系统级对话框（文件选择等），后续由平台层提供。
