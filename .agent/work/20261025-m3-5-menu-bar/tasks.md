# 20261025-m3-5-menu-bar 任务清单

## MenuBar

- [x] T01 `MenuItem`（label/shortcut/enabled/separator/action）+ 静态构造
- [x] T02 `MenuBar`：标题测量/绘制、点击切换、hover 切换
- [x] T03 overlay：`MenuScrim`（吞点击、转发标题 hover）+ `MenuPopup`（高亮、快捷键、分隔线、禁用态）
- [x] T04 键盘：Down/Enter 打开、Left/Right 切菜单、Up/Down 跳过分隔与禁用项、Home/End、Esc
- [x] T05 激活语义：先 close 再执行 action；close 幂等；延迟销毁
- [x] T06 新增 `test_ui_menu_bar`（点击开关、点击项回调、键盘、禁用项、遮罩关闭）
- [x] T07 gallery 新增 Menu 分区（File/Edit/View + 状态回显 + COSINE_GALLERY_MENU）
- [x] T08 全量回归（24/24 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成并关闭；决策 #46
