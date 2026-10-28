# 20261028-editor-linking 任务清单

- [x] T01 `DragFloat`：`on_drag_begin` / `on_drag_end`
- [x] T02 `SetNodeFieldCommand`（显式 before/after + writer）
- [x] T03 属性面板 10 行接入命令（拖动合成一条 undo）
- [x] T04 `TimelineView::set_selected_node` + 轨道行高亮
- [x] T05 场景树选中 → 属性面板 → 时间轴 三向联动（树为源）
- [x] T06 Edit 菜单 Undo/Redo 接 `CommandStack`
- [x] T07 `test_set_node_field_command`；ctest 56/56
- [x] T08 冒烟与导出验证（exit=0 / validation=0）
- [ ] T09 `Splitter` 可拖分区
- [ ] T10 时间轴编辑（拖动/删除/插值）入命令栈
- [ ] T11 颜色选择与键盘微调入命令栈
- [ ] T12 预览时间标尺与逐帧
- [ ] T13 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义（本阶段）

- 拖动数值框改属性后，Edit → Undo 能回到拖动前，Redo 能恢复。
- 场景树选中节点时，时间轴对应轨道行高亮。
