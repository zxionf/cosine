# 20261027-ui-dpi-scale 交接

## 结果

引入 `ui_scale`（逻辑像素 → framebuffer 像素倍数），**默认 1.0，行为不变**：

| 层 | 改动 |
|---|---|
| `FontAtlas` | `set_pixel_ratio()` / `glyph_pixel_size(size)`；字形按 `size × ratio` 栅格化；量化上限 256 |
| `Renderer` | `set_ui_scale()`；push constant `scale = 2/(extent/ratio)`；scissor 乘比例；`set_font_pixel_ratio()` |
| `UISystem` | `set_ui_scale()`；root 尺寸 = `extent / ratio` |
| `Application` | `COSINE_UI_SCALE=auto\|数值`；输入坐标 ÷ ratio；注入三处 |

新增 `test/test_font_atlas_dpi.cpp`（3 个用例）。

## 验证

- `ctest` **51/51**（build 与 ASAN）。
- `COSINE_UI_SCALE=2 ./ui_gallery`：exit=0、validation=0；截图 UI 放大两倍、字形清晰。
- `sele close` 为 `appended`（`DECISIONS #125`）；阶段计划 M4.6 的 T8 已勾选。

## 怎么用

```bash
# 跟随系统 DPR（X11 下通常 1.0）
COSINE_UI_SCALE=auto ./ui_gallery

# 强制 2 倍
COSINE_UI_SCALE=2 ./ui_gallery
COSINE_UI_SCALE=2 ./ui_demo
```

## 关键点

- **字形必须按物理像素栅格化**（`size × ratio`），否则投影放大后是位图拉伸，糊。
- 绘制尺寸仍用**逻辑字号**，`width_ratio` 等比例量不变，所以测量/布局无感。
- scissor 与 viewport 分属两套坐标：viewport 物理，scissor 从逻辑乘回物理。

## 后续

- 运行中 DPI 变化（拖到另一块屏）需重启，未接 `glfwSetWindowContentScaleCallback`。
- 下一步：T5 字距 / T9 TextSpan / T10 字体覆盖 / T11 IME / T12 SDF / T13 TreeView，或 M5。
