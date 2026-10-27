# 20261027-ui-dpi-scale

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.6 T8：字号与 DPI

### 现状

UI 坐标 = framebuffer 像素：`UISystem` 直接用 `extent` 作 root 尺寸，
`Renderer` 的 push constant `scale = 2/extent`，输入经 `framebuffer_scale()` 后不再缩放。
DPR > 1 时 UI 显得过小。

### 改动

引入 `ui_scale`（逻辑像素 → framebuffer 像素的倍数），**默认 1.0，行为完全不变**：

1. `Application`：`COSINE_UI_SCALE=auto`（跟随 `framebuffer_scale()`）或数值（0.25–4），
   构造末尾注入 Renderer / UISystem / 字体图集。
2. `UISystem::set_ui_scale`：root 尺寸 = `extent / ui_scale`。
3. `Renderer::set_ui_scale`：
   - push constant `scale = 2 / (extent / ui_scale)`（viewport 仍是物理像素）；
   - 批次 scissor 坐标乘 `ui_scale` 再取整。
4. 输入：`cursor_to_framebuffer(...) / ui_scale`（三处回调）。
5. `FontAtlas::set_pixel_ratio`：字形按 `size * ratio` 栅格化（`glyph_pixel_size()`），
   绘制仍用逻辑字号；`quantize_size` 上限 160 → 256。
6. `text_debug` 的字形查找同步改用 `glyph_pixel_size()`。

### 为什么字形要按物理像素栅格化

投影会放大 `ui_scale` 倍。若字形仍按逻辑字号栅格化，等于把位图放大——模糊。
按 `size * ratio` 栅格化后，绘制尺寸仍是逻辑值，投影放大后正好落在物理像素上，清晰。

### 验证

- `test_font_atlas_dpi.cpp`（3 用例）：比例 1 / 2 / 1.5 下 loader 收到的物理字号；
  非法比例回落 1.0；量化与 `[4,256]` 上限。
- `ctest` **51/51**（build 与 ASAN）。
- `COSINE_UI_SCALE=2 ./ui_gallery`：exit=0、validation=0；截图 UI 放大两倍且文字清晰。

### 边界（本次不做）

- 默认 1.0，不改变现有行为。
- `auto` 依赖 `glfwGetFramebufferSize / glfwGetWindowSize`；X11 下通常为 1.0。
- 不做「按 DPI 选不同字号档位」（`quantize_size` 仍是 1px 粒度，非整数比例靠量化吸收）。
- 未处理运行中 DPI 变化（`glfwSetWindowContentScaleCallback`），切换需重启。
