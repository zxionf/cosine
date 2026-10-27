# 20261027-label-measure-cache

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.6 T3：字形缓存命中与测量性能

### 现状

- 字形本身的命中已经是 O(1)（`glyphs_` 哈希表），**真正的开销在测量**。
- `Label::measure()` 每次逐 codepoint 查表累加（换行时逐行计算）。
- `Label::append_to()` **每帧**都调 `atlas->measure(...).x`，即使左对齐根本不需要文本宽度。

### 改动

1. `Label::append_to()`：**左对齐直接跳过测量**（`offset_x = 0`），只有居中/右对齐才取文本宽度。
2. `Label::measure()` 与 `text_width()` 共用一份测量缓存，key =
   `(text, size, wrap_width, line_spacing, max_lines, wrap, atlas)`。
3. 影响测量的 setter（`set_text` / `set_text_size` / `set_wrap` / `set_max_lines` /
   `set_ellipsis` / `set_line_spacing`）都会 `invalidate_measure()`。
4. **缓存 key 含图集指针**：切换 `active_font_atlas` 后自动失效——否则换字体后仍返回旧宽度。

### 验证

新增 `test_ui_label_measure.cpp`（6 个用例）：

- 切换图集（advance 0.6 ↔ 0.9）后测量跟随变化，切回能重新算对；
- 改 text / size 后失效；
- 改 line_spacing / max_lines / wrap 后失效；
- 左对齐绘制原点仍是 `pos + bearing`（不测量也不影响对齐）；
- 居中偏移与测量宽度一致。

`ctest` **49/49**（build 与 ASAN）；gallery 冒烟 exit=0 / validation=0，截图文字位置正常。

### 边界（本次不做）

- 缓存只有 1 条（最近一次）。Label 通常被一个父级以固定约束测量，单条足够；
  多约束交替时会退化为「每次重算」，但仍不产生错误。
- 不做跨 Label 的共享测量缓存（图集层面）。
- 不做字形位图缓存（字形已在图集中）。
