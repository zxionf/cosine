# 20261025-text-display-lab 任务清单

## 文字调试与排版组件

- [x] T01 `graphics/text_debug.hpp`：TextDebugOptions + append_text_debug（字形盒/行框/基线/advance，支持换行）
- [x] T02 `Label`：text debug、TextAlign、像素对齐、行距
- [x] T03 `ui/typography.hpp`：TextRole / text_style / Heading / Paragraph
- [x] T04 `ui_demo` 改为文字实验台（样例 + F1–F5 调试层，COSINE_TEXT_DEBUG/COSINE_TEXT_SNAP）
- [x] T05 新增 `test_text_display`（调试盒与字形重合、行距测量、换行一致、对齐、像素对齐、角色尺寸）
- [x] T06 全量回归（21/21 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成：21/21 ctest、ASAN、lavapipe 冒烟（ui_demo + ui_gallery）通过
- 修复：换行宽度未受 max_size 约束导致测量/布局不一致；调试层未按 wrap 折行导致盒线错位
