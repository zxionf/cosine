# 20261027-timeline-view 任务清单

- [x] T01 `Track::set_key_time`（保序重插）+ 非 const `tracks()`
- [x] T02 `TimelineView`：标尺刻度、轨道行、关键帧菱形、播放头
- [x] T03 点击 seek + 拖动关键帧（`on_seek` / `on_key_moved` 回调）
- [x] T04 编辑器接入：替换占位、播放头循环、状态栏提示
- [x] T05 编辑器 `COSINE_EXPORT_PNG`（显式固定导出尺寸）
- [x] T06 布局修复：树行高 46、属性面板改 `ScrollPanel`、时间轴 8px 边距
- [x] T07 `test/test_ui_timeline_view.cpp`（4 组）+ 注册；ctest 56/56
- [ ] T08 框选多选、吸附、右键删除 / 插值切换
- [ ] T09 与 `Player` 联动（播放时游标跟随、seek 驱动预览）
- [ ] T10 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义（本阶段）

- 时间轴能显示多轨与关键帧，能拖关键帧改时间、点标尺跳转。
- 编辑器可通过 `COSINE_EXPORT_PNG` 离屏导出界面用于验收。
