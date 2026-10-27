# 20261027-label-measure-cache 交接

## 结果

`src/ui/label.hpp`：

1. `append_to()`：**左对齐跳过测量**（`offset_x = 0`），只有居中/右对齐才调 `text_width()`。
2. `measure()` 与 `text_width()` 共用单条测量缓存，key =
   `(text, size, wrap_width, line_spacing, max_lines, wrap, atlas)`。
3. 影响测量的 setter 都调 `invalidate_measure()`。
4. **key 含图集指针**——切换 `active_font_atlas` 后自动失效。

新增 `test/test_ui_label_measure.cpp`（6 个用例，注册为 `ui_label_measure`）。

## 验证

- `ctest` **49/49**（build 与 ASAN）。
- gallery 冒烟：exit=0、validation=0；截图文字位置正常。
- `sele close` 为 `appended`（`DECISIONS #123`）；阶段计划 M4.6 的 T3 已勾选。

## 关键细节

- 缓存只有 1 条。`measure()` 和 `append_to()` 的 key 不同（后者 `wrap_width = -1`），
  会互相覆盖——这是有意的：两者不会在同一次布局里交替使用同一 Label 的同一 key。
- **图集指针必须在 key 里**，否则换字体后测量结果会停留在旧值。

## 后续

- 若将来出现「同一 Label 被多个父级以不同约束测量」，可把单条缓存扩成小 LRU。
- 下一步：**T4 基线/行高语义收紧**，或切回 **M5**。
