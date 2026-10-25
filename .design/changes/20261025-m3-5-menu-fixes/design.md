# 20261025-m3-5-menu-fixes

<!-- 为什么改、边界与不属于本次的范围 -->

## 菜单可见性修复

1. **菜单项文字垂直居中**：行内文字原来用 `text_size`（16）当高度参与居中，
   实际行盒是 `line_height`（≈23），导致文字整体偏下。改为按 `FontAtlas::line_height(size)` 居中。
   同一问题也存在于单行 `TextInput`（`y + (h - font_size)/2`），一并改为按行高居中。
2. **弹层上下留白对称**：`popup_height` 原来只加了一次 `padding`（顶部留白、底部贴边），
   改为 `padding * 2`，最后一行与底边之间有空隙。
3. **悬停切换菜单后内容不更新**：`MenuPopup` 在创建时固定了 `menu_index_`，
   切换菜单只更新了几何尺寸。新增 `MenuPopup::set_menu_index()`，
   在 `update_popup_geometry()` 里同步，内容随标题切换。

## 测试

- `test_ui_menu_bar` 新增：
  - 悬停切换后点击首行触发的是新菜单的项（Edit > Undo，而不是 File > New）
  - 弹层高度 = 行高之和 + `padding * 2`（上下对称）

## 边界

- 不改变菜单位置策略（仍在标题下方，不做边界翻转）。
- 不改菜单项字号/间距，仅修正垂直居中基准。
