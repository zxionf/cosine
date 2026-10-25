# 20261025-m3-5-menu-popup-size 任务清单

- [x] T01 复现：headless 断言切换菜单后弹层尺寸变化（原实现失败）
- [x] T02 定位根因：`MenuPopup` 构造设置 min/max_size + `layout_child` 的 `clamp_size`
- [x] T03 移除构造期的尺寸约束，改由 `arrange` 写入 `local_/global_`
- [x] T04 `test_ui_menu_bar`：高度变化 + 宽度随内容超过 160px
- [x] T05 全量回归（24/24 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成并关闭；决策 #49
