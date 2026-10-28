# 20261028-value-commits 任务清单

- [x] T01 `DragFloat::set_on_commit` + `nudge()`（键盘微调上报）
- [x] T02 `ColorInput::set_on_commit`（点击色块 / 键盘切色上报）
- [x] T03 `SetNodeColorCommand`（vec4 专用）
- [x] T04 编辑器：属性行与颜色行接入 commit
- [x] T05 颜色 commit 对齐 alpha（不把半透明改成不透明）
- [x] T06 测试：`test_keyboard_commit` × 2、`test_set_node_color_command`
- [x] T07 回归：ctest 56/56；冒烟 exit=0 / validation=0
- [ ] T08 `Splitter` 可拖分区
- [ ] T09 预览时间标尺与逐帧
- [ ] T10 框选追加、批量拖动合成单条命令
- [ ] T11 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义

- 键盘微调数值、点色块改颜色都能用 Edit → Undo 回滚。
- 颜色撤销不会改变节点的 alpha。
