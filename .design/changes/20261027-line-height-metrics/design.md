# 20261027-line-height-metrics

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.6 T4：基线/行高语义收紧

### 现象

`FontFace::metrics()` 直接用 FreeType 的 `size->metrics.height` 作为行高，只钳了下限 `1.0`。
对部分字体（尤其 CJK），`height` 可能小于 `ascender + |descender|`，
降部（g / j / p / q / y）就会压在行框或控件边界上，被裁切或贴着边。

### 改动

1. 新增 `FontAtlas::normalize_metrics(Metrics)`：
   `line_height_ratio = max(line_height_ratio, ascent_ratio - descent_ratio, 1.0)`。
   `descent_ratio` 按 FreeType 约定为负，所以用减号得到字形纵向范围。
2. `FontAtlas::begin_generated()` 在存入 metrics 前归一化。
3. `FontFace::metrics()` 改用 `normalize_metrics`（去掉本地 clamp）。

### 效果

- 所有派生量自动覆盖完整字形范围：`line_height` / `line_step` / `measure` 的多行高度 /
  行框（`TextDebugOptions::line_box`）/ TextInput 行高 / 各控件垂直居中。
- `ui_demo`（XEL Text Lab）里 96px 的 “Hqyp gjpqy”：`COSINE_TEXT_DEBUG=1` 打开行框后，
  行框下边界明显低于降部最低点。

### 验证

`test/test_font_atlas_metrics.cpp`（4 个用例）：

- 行高小于字形范围时被抬高到 `ascent - descent`；
- 已经足够大的行高保持不变；
- 下限 `1.0`；
- `begin_generated` 归一化后 `line_height`、`measure` 单行/多行高度一致。

`ctest` **50/50**（build 与 ASAN）；`ui_demo` 校验错误 0。

### 预期布局变化

行高可能变大（取决于字体），因此多行文本、TextInput、列表行高会略微增高。
这是本次的**目标效果**，不是回归。

### 边界（本次不做）

- 未引入独立的 `line_gap` / `baseline_offset` 配置。
- 不改变字形实际绘制基线（仍按 `bearing_y` 定位）；本项只保证行框容得下字形。
