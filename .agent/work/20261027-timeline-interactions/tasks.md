# 20261027-timeline-interactions 任务清单

- [x] T01 拖动吸附（整秒 / 其它关键帧，6px 阈值）
- [x] T02 右键删除关键帧（空轨道一并移除）
- [x] T03 双击循环切换插值
- [x] T04 `TimelineView::set_on_changed` 回调
- [x] T05 编辑器接入 `Player`：Play/Pause/Reset + tick 驱动预览与游标
- [x] T06 `COSINE_EXPORT_TIME`（导出前推进 N 秒）
- [x] T07 测试 +3 组；ctest 56/56
- [x] T08 导出验证：播放 1.2s 后 Card 移动、游标跟随
- [x] T09 框选多选与批量移动（含防碰撞排序、选中状态同步）
- [x] T12 存盘往返验收：`test_edit_then_round_trip`
- [ ] T10 时间轴编辑接 `CommandStack`（可并入 M6.4b）
- [ ] T11 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义

- 时间轴可拖动（带吸附）、右键删除、双击改插值。
- 播放按钮驱动预览与游标，Reset 归零。
