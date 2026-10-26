# 20261026-absolute-zindex 交接

## 结果

- `Widget` 新增：`set_absolute(bool)` / `absolute()`、`set_z_index(int)` / `z_index()`、
  `paint_order()` / `overlay_paint_order()`（按 z 稳定升序，无非零 z 时保持原序）。
- `Vbox` / `Hbox` / `Grid` / `Wrap`：`measure` 跳过绝对定位子项（不计入尺寸/行数/折行），
  `arrange` 单独按 `(内容区原点 + x(), 内容区原点 + y())` 摆放，`append_to` 用 `paint_order()`。
- `UIRoot::append_to` 的 children 与 overlays 各自按 `paint_order()`；
  `UISystem` 的命中（降序）、更新、Tab 收集统一用同一顺序。
- gallery `Layout` 新增：Panel 卡片右下角绝对定位角标（z-index 10）。
- 新增 `test/test_ui_absolute_zindex.cpp`（7 个用例），注册为 `ui_absolute_zindex`。

## 验证

- `ctest --test-dir build`：39/39；`ctest --test-dir build-asan`：39/39。
- gallery 冒烟 Layout 分区（默认 / classic / 滚动 700）：`validation=0`。
- `sele lint` 四项 ok。

## 实现要点

- 绝对定位坐标相对**父容器内容区**（padding 内），不是父容器外框。
- 绝对定位子项尺寸仍走 `Widget::measure_child`，因此支持百分比与宽高比。
- 绝对定位子项不参与 `align` / `stretch`：位置完全由坐标决定。
- z-index：绘制升序（大的在上）、命中降序（大的先命中）；`overlays` 永远在 `children` 之上。
- `paint_order()` 在没有任何非零 z 时直接返回原序，避免每帧 `stable_sort`。
- 未接入 z-index 的容器（Panel / TabBar / Table / TreeView / ScrollView 等）仍按加入顺序绘制。

## 后续

- M3.5 布局补充四项已全部完成（Grid / Wrap / 百分比与宽高比 / 绝对定位与 z-index）。
- 剩余 M3.5 可选项：`Image` 控件占位接口（等 M6 资源管线）、组件单测补齐。
- 之后进入 M4。
