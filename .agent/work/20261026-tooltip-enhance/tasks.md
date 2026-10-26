# 20261026-tooltip-enhance 任务清单

- [x] T01 `Tooltip::set_delay` + 悬停计时（不足阈值不显示，离开重置）
- [x] T02 显示后跟随光标
- [x] T03 边界翻转与夹取（右下溢出翻到左上）
- [x] T04 gallery 接入：`update_tooltip(cursor, dt)` 传入 root bounds
- [x] T05 新增 `test_ui_tooltip`（延迟、跟随、翻转、离开重置）
- [x] T06 全量回归：构建 + ctest 27/27（ASAN 亦通过）
- [x] T07 gallery 多目标提示：`TipTarget` 列表（watermark/按钮/边界色条），每个目标独立文本；顺带修复 `tip_button` 被后写赋值覆盖
- [x] T08 gallery TreeView 页补充键盘与交互提示

## 完成定义

- 构建与 ctest 全绿，gallery 中 tooltip 行为可人工验收。
- SPEC 更新，决策记录延迟/翻转的取值与接口。
