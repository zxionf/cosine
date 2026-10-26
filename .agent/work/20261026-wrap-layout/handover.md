# 20261026-wrap-layout 交接

## 结果

- 新增 `Wrap` 流式布局容器：`src/ui/wrap.hpp`、`src/ui/wrap.cpp`，已在 `src/CMakeLists.txt` 注册。
- 子项按可用宽自动折行；`row_align`（Start/Center/End/SpaceBetween）与 `cross_align`
  控制行内水平分布与垂直对齐；单个超宽子项独占一行。
- gallery `Layout` 分区新增 12 个标签按钮的自动折行示例（宽度固定 620）。
- 新增 `test/test_ui_wrap.cpp`（9 个用例），注册为 `ui_wrap`。

## 验证

- `ctest --test-dir build`：37/37；`ctest --test-dir build-asan`：37/37。
- gallery 冒烟 Layout 分区（默认 / `COSINE_GALLERY_CLASSIC=1` / 滚动 600）：`validation=0`。
- `sele lint` 四项 ok。

## 实现要点

- 折行：按子项顺序累加「含 margin 的宽 + 间距」，放不下且当前行非空时换行；
  单个子项宽于整行时独占一行，x 不为负。
- 行高 = 行内子项高（含上下 margin）最大值；行内按 `cross_align` 垂直摆放。
- 行内剩余宽按 `row_align` 分配：Center 居中、End 靠右、SpaceBetween 均分到间隙（≥2 个子项）。
- **measure 在有限约束下返回 `max(constraints.x, 内容宽)`**：折行依赖宽度，必须保证
  measure 与 arrange 用同一宽度，否则行数不同会导致高度错位。无界约束下返回内容宽（单行）。
- 只使用子项 `Align` 的 y 分量与 `stretch_y`，忽略 `a.x`/`stretch_x`（水平位置由流式排布决定）。
- 忽略 `flex_grow`。

## 后续

- M3.5 布局补充下一项：百分比尺寸与 aspect ratio（需要改 `measure` 的约束语义，影响面较大）；
  或绝对定位 / z-index。
- 之后可转 `Image` 控件占位接口（等 M6 资源管线）。
