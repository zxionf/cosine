# 20261026-progress-submenu 任务清单

- [x] T01 `Progress::set_indeterminate` + `indeterminate()` + `on_update(dt)` 推进 phase
- [x] T02 Progress 不确定态绘制：accent 条带按相位滑动，`radius_pill` 圆角
- [x] T03 `MenuItem::children` + `MenuItem::submenu(...)` + `has_submenu()`
- [x] T04 `MenuPopup` 通用化（持有 items 拷贝），有子菜单的行画右三角
- [x] T05 `MenuBar` 管理子 popup 栈：hover 打开/切换/逐层关闭、定位与右边界翻转、整条链关闭
- [x] T06 键盘 Right/Left 逐层进出、Up/Down/Home/End 在活动层移动、Enter 执行或展开、Esc 全关
- [x] T07 gallery：indeterminate Progress 示例；Edit 菜单增加三层子菜单
- [x] T08 `test_ui_progress` + `test_ui_submenu`
- [x] T09 全量回归：ctest 35/35 + ASAN 35/35 + gallery 冒烟 4 组（validation=0）
- [x] T10 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- Progress 可在确定/不确定之间切换，不确定态条带循环滑动且随真实 dt 推进。
- 菜单支持一层子菜单：鼠标 hover 打开、点击执行并关闭、Esc/点外部关闭、Right/Left 键盘开关。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验收。
