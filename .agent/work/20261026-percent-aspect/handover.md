# 20261026-percent-aspect 交接

## 结果

- `Widget` 新增尺寸规格：`set_size_percent(glm::vec2)`（每轴占父级可用空间比例）与
  `set_aspect_ratio(float)`（宽/高），字段 `size_percent_` / `aspect_ratio_`。
- 新增统一入口 `static Widget::measure_child(child, constraints)`：
  百分比 → 比例轴取约束值 → 宽高比 → `clamp_size`。
- `Vbox` / `Hbox` / `Grid` / `Wrap` 的子项测量全部改用 `measure_child`，
  并在 arrange 的 stretch 分支加上「该轴已声明百分比则不 stretch」。
- gallery `Layout` 新增：16:9 预览卡（宽 40%）与 50% 宽色块。
- 新增 `test/test_ui_percent_aspect.cpp`（10 个用例），注册为 `ui_percent_aspect`。

## 验证

- `ctest --test-dir build`：38/38；`ctest --test-dir build-asan`：38/38。
- gallery 冒烟 Layout 分区（默认 / classic / 滚动 900）：`validation=0`。
- `sele lint` 四项 ok。

## 实现要点

- 百分比相对**父级给出的可用空间**（已扣 margin），不是相对父容器总尺寸。
- 该轴约束无界（`>= 1e8`）时百分比不生效，退回子项自然尺寸。
- 宽高比：只声明了百分比高（且没有百分比宽）时用高推宽，其余用宽推高。
- 百分比优先于 stretch：`set_size_percent({0.5, 0})` + `h_stretch` 仍是半宽；
  位置仍由 align 决定。
- `min_size` / `max_size` 最后仍 clamp。
- 未声明时行为与迁移前完全一致（原有 37 个测试全绿）。

## 未接入（语义不同）

- `ScrollView` / `TabView` / `Dialog` 测量的是「内容自然尺寸」，本次未改；
  它们内部的 Vbox/Grid 等仍会正常解析子项百分比。

## 后续

- M3.5 布局补充最后一项：绝对定位与 z-index（涉及 `UIRoot` overlay 与绘制顺序）。
- 或转 `Image` 控件占位接口（等 M6 资源管线）。
