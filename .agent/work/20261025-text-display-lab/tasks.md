# 20261025-text-display-lab 任务清单

## 文字调试与排版组件

- [x] T01 `graphics/text_debug.hpp`：TextDebugOptions + append_text_debug（字形盒/行框/基线/advance，支持换行）
- [x] T02 `Label`：text debug、TextAlign、像素对齐、行距
- [x] T03 `ui/typography.hpp`：TextRole / text_style / Heading / Paragraph
- [x] T04 `ui_demo` 改为文字实验台（样例 + F1–F5 调试层，COSINE_TEXT_DEBUG/COSINE_TEXT_SNAP）
- [x] T05 新增 `test_text_display`（调试盒与字形重合、行距测量、换行一致、对齐、像素对齐、角色尺寸）
- [x] T06 全量回归（21/21 ctest、ASAN、lavapipe 冒烟）
- [x] T07 按字号栅格化字形（FontAtlas glyph key = 字号 + codepoint；FontFace 按目标 px 重建）
- [x] T08 裁剪矩形向外取整，修复降部被截一行
- [x] T09 `test_text` 增加按字号栅格化用例

## 当前进度

- 完成：21/21 ctest、ASAN、lavapipe 冒烟通过
- 根因：16px 基准图集被放大绘制，导致描边模糊、降部看似缺失（非裁剪问题）
- [x] T10 后续优化 backlog 写入 `.design/阶段计划.md`（T1–T12：图集淘汰/上传/缓存/kerning/shaping/IME/SDF 等）
