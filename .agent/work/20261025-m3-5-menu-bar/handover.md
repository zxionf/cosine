# 20261025-m3-5-menu-bar handover

## 交付

- `src/ui/menu_bar.hpp`：`MenuItem`、`MenuBar`、`detail::MenuScrim`、`detail::MenuPopup`
- `test/test_ui_menu_bar.cpp`：打开/关闭、点击项、键盘导航、禁用项、遮罩关闭
- `examples/ui_gallery`：新增 Menu 分区（第 6 个），`COSINE_GALLERY_MENU=1` 启动即展开 File 菜单

## 关键结论

- scrim 必须把标题区 hover 转发给 MenuBar，否则打开菜单后无法在不同菜单间滑动切换。
- 激活项要「先 close 再 action」，否则回调里打开 dialog / 修改菜单会与 overlay 生命周期打架。
- 键盘高亮需跳过分隔线与禁用项，Home/End 直达首尾可用项。

## 回归

24/24 ctest；ASAN 24/24；lavapipe 冒烟通过。

## 后续

M3.5 剩余：Table/TreeView、Tooltip 延迟/跟随/翻转、Grid/Wrap 布局、控件状态样式统一与过渡、菜单子菜单。
