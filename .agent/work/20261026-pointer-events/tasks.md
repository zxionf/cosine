# 20261026-pointer-events 任务清单

- [x] T01 `Widget` 三个虚钩子（`on_right_click` / `on_double_click` / `on_long_press`），默认不消费
- [x] T02 `UISystem::dispatch_mouse_button` 按钮语义：右键不抢焦点、不走 `on_click`，改发 `on_right_click`
- [x] T03 双击判定：`elapsed_` 累加 + 目标/按钮/时间/位置容差，触发后清空避免三击连发
- [x] T04 长按：`tick(dt)` 驱动、移出取消、只触发一次；`set_double_click_ms/slop`、`set_long_press_ms`
- [x] T05 `TreeView` 双击行切换展开（箭头区交由单击）；gallery `Inputs` 分区新增 `Pointer events` 探针
- [x] T06 `test_ui_pointer_events`：左键/右键/双击阈值与容差/长按一次性与取消/消费冒泡/阈值可配置
- [x] T07 全量回归：构建 + ctest 30/30 + ASAN 30/30 + gallery 冒烟 3 组（validation=0）
- [ ] T08 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- 右键不再抢焦点、不再触发表面上的 `on_click`；控件可以通过 `on_right_click` 响应。
- 双击与长按按配置的阈值触发，纯逻辑测试可稳定复现。
- gallery 里能人工验证四种指针事件（click / double / right / long）。
- 构建零新增警告，全部测试与 ASAN 通过。
