# 20261028-timeline-commands 任务清单

- [x] T01 `Track::find_key(time)`
- [x] T02 `MoveKeyCommand`（apply 幂等、revert 反向）
- [x] T03 `SetKeyEasingCommand`
- [x] T04 `TimelineView` 上报回调：拖动结束 / 右键删除 / 双击切插值
- [x] T05 编辑器三条回调接入 `CommandStack`
- [x] T06 更新时序测试断言（回调参数 + 数据未直接变）
- [x] T07 回归：ctest 56/56；冒烟 exit=0 / validation=0
- [ ] T08 `Splitter` 可拖分区
- [ ] T09 颜色选择与键盘微调入栈
- [ ] T10 预览时间标尺与逐帧
- [ ] T11 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义

- 拖动关键帧、右键删除、双击切插值都能用 Edit → Undo 回滚。
- 时序控件不再直接改数据（删除/插值），拖动保留即时预览。
