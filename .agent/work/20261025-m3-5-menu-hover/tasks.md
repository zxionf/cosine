# 20261025-m3-5-menu-hover 任务清单

- [x] T01 定位根因：`update_popup_geometry` 每帧调 `set_menu_index` 清 hover；`arrange` 通过 setter 标脏导致每帧重排
- [x] T02 `set_menu_index` 仅在菜单变化时清 hover
- [x] T03 `MenuPopup::arrange` 直接写 `local_/global_`，不再标脏
- [x] T04 `update_popup_geometry` 改为 `set_menu_index + arrange`
- [x] T05 `test_ui_menu_bar`：布局后 hover 保持 + 高亮进入 DrawList
- [x] T06 反向验证：去掉守卫测试立即失败
- [x] T07 全量回归（24/24 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成并关闭；决策 #48
