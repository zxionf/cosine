# 20261026-absolute-zindex 任务清单

- [x] T01 `Widget`：`absolute_` / `z_index_` 字段与 setter，`paint_order()` 稳定排序
- [x] T02 `Vbox` / `Hbox`：measure 跳过绝对定位子项，arrange 单独定位，append_to 用 paint_order
- [x] T03 `Grid` / `Wrap`：同上（不占格、不参与折行）
- [x] T04 `UIRoot::append_to` 与 `UISystem`（命中、更新、Tab 收集）统一用 paint_order
- [x] T05 `test/test_ui_absolute_zindex.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T06 gallery `Layout` 分区：卡片右下角绝对定位角标
- [x] T07 全量回归：ctest 39/39 + ASAN 39/39 + gallery 冒烟 3 组（validation=0）
- [x] T08 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- 任意容器内可放「角标/浮层」，不参与流式布局，尺寸支持百分比与宽高比。
- z-index 同时影响绘制与命中，顺序一致；默认全为 0 时行为与迁移前一致。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验收。
