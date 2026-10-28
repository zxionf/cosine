# 20261028-splitter-panes 任务清单

- [x] T01 三份嵌套 `Splitter`（上下 / 左右 / 左右）
- [x] T02 `build_property_panel` 返回 `unique_ptr`，由 Splitter 接管
- [x] T03 各 pane 的 `min_size`（tree/preview/property/timeline）
- [x] T04 `layout()` 简化为定位分割器与顶栏
- [x] T05 修复首帧 `root` 尺寸为 0 导致的 `ScrollView` 崩溃（splitter 初始下限）
- [x] T06 回归：ctest 56/56；冒烟 exit=0 / validation=0；导出确认布局
- [ ] T07 预览时间标尺与逐帧
- [ ] T08 框选追加、批量拖动单条命令
- [ ] T09 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义

- 三栏与时间轴都能拖动分隔条调整比例，且不会把面板压到不可用。
- 启动、冒烟、导出都不崩溃。
