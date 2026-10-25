# 20261025-text-display-lab handover

## 结论

文字实验台、文字调试层、排版组件与「按字号栅格化」全部完成并通过回归。

## 交付

- `src/graphics/text_debug.hpp`：`TextDebugOptions` + `append_text_debug`（字形盒/行框/基线/advance，支持换行与行距）
- `src/ui/label.hpp`：`TextAlign`、`set_pixel_snap`、`set_line_spacing`、`set_text_debug`
- `src/graphics/font_atlas.*`：`line_spacing` 参数贯通 measure/append；glyph key = (字号, codepoint)；`quantize_size`
- `src/graphics/font_face.*`：`add_glyph(atlas, cp, pixel_size)` 按目标尺寸重建，缓存基准 metrics
- `src/ui/typography.hpp`：`TextRole` / `text_style` / `Heading` / `Paragraph`
- `src/backend/vulkan/renderer.cpp`：裁剪矩形向外取整
- `examples/ui_demo`：文字实验台（F1–F5、COSINE_TEXT_DEBUG / COSINE_TEXT_SNAP）
- `test/test_text_display.cpp`、`test/test_text.cpp`（按字号栅格化）、`test/test_ui_demo.cpp`

## 关键结论

- 降部“缺失”的真实原因不是裁剪，而是 16px 图集被放大绘制；逐字号栅格化后解决。
- 换行宽度必须同时受 `constraints.x` 与 `Label::max_size` 约束，否则测量与 arrange 不一致。
- 调试层必须复用换行规则，否则盒线与文本错位。

## 回归

- `ctest` 21/21；ASAN 21/21；lavapipe 冒烟（ui_demo + ui_gallery）通过

## 后续

文本优化 backlog 已写入 `.design/阶段计划.md`（T1–T12）。
