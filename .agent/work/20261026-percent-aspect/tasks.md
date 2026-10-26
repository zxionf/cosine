# 20261026-percent-aspect 任务清单

- [x] T01 `Widget`：`size_percent_` / `aspect_ratio_` 字段与 setter（`set_size_percent` / `set_aspect_ratio`）
- [x] T02 `Widget::measure_child(child, constraints)`：百分比 → 比例轴 → 宽高比 → clamp
- [x] T03 `Vbox` / `Hbox` 的 measure 与 arrange 接入 `measure_child`（含百分比优先于 stretch）
- [x] T04 `Grid` / `Wrap` 接入 `measure_child`
- [x] T05 `test/test_ui_percent_aspect.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T06 gallery `Layout` 分区：16:9 预览卡 + 50% 宽色块
- [x] T07 全量回归：ctest 38/38 + ASAN 38/38 + gallery 冒烟 3 组（validation=0）
- [x] T08 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- 任意控件可声明「占父级的一半宽」或「16:9」，在 Vbox/Hbox/Grid/Wrap 中生效。
- 无界约束下自动退回自然尺寸，不会产生 0 或 NaN。
- 未声明时行为与迁移前完全一致。
- 构建零新增警告，全部测试与 ASAN 通过。
