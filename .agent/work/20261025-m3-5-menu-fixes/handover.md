# 20261025-m3-5-menu-fixes handover

## 三个问题与修法

1. 菜单项文字不居中
   - 原因：行内文字用 `text_size`（16）作为高度参与垂直居中，实际行盒是 `line_height`（≈23）。
   - 修法：`MenuPopup` 统一按 `FontAtlas::line_height(size)` 居中；单行 `TextInput` 同类问题一并修正。
2. 弹层上留白、下贴边
   - 原因：`popup_height` 只加了一次 `padding`。
   - 修法：`padding * 2`，最后一行与底边留出对称空隙。
3. 悬停切换菜单后内容不换
   - 原因：`MenuPopup` 创建时固定 `menu_index_`，切换只更新几何。
   - 修法：新增 `MenuPopup::set_menu_index()`，在 `update_popup_geometry()` 中同步。

## 验证

- `test_ui_menu_bar`：悬停切换后首行点击触发新菜单项（Edit > Undo）；弹层高度 = 行高和 + `padding * 2`。
- 24/24 ctest；ASAN 24/24；lavapipe 冒烟通过。
- 截图：File/Edit 菜单文字居中、上下留白对称、`COSINE_GALLERY_MENU=<index>` 可直接打开指定菜单。
