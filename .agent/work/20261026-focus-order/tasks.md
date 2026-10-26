# 20261026-focus-order 任务清单

- [x] T01 `Widget::set_tab_order(int)` / `tab_order()`；`collect_focusable` 保留全部可聚焦控件（用于定位），Tab 候选过滤 `tab_order < 0`
- [x] T02 `focus_next` 按「显式 order 优先 + 同类保持树序」稳定排序；焦点在 `tab_order < 0` 控件上时用树序定位下一个停靠点
- [x] T03 `RadioGroup`：`focusable()`、`set_options` 给 Radio 设 `tab_order(-1)`、方向键/Home/End 切换 + 组内焦点跟随
- [x] T04 gallery：`Inputs` 加 `Tab order (3 → 1 → 2)` 演示；Radio group 标题补方向键与单停靠点说明
- [x] T05 `test_ui_focus_order`：默认树序、显式顺序、反向、负值跳过、混排、RadioGroup roving 与 Tab 跳转
- [x] T06 全量回归：ctest 32/32 + ASAN 32/32 + gallery 冒烟 3 组（validation=0）
- [x] T07 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- Tab 顺序默认与现在完全一致；设置 `tab_order` 后按数值遍历，负值跳过。
- RadioGroup 只占一个 Tab 停靠点，Up/Down/Home/End 可在组内切换。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验证 Tab 顺序。
