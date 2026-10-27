# 20261027-ui-dpi-scale 任务清单

- [x] T01 `FontAtlas::set_pixel_ratio` + `glyph_pixel_size()`（量化上限 256）
- [x] T02 `Renderer::set_ui_scale`：push constant 用逻辑尺寸、scissor 乘比例
- [x] T03 `UISystem::set_ui_scale`：root 尺寸 = extent / scale
- [x] T04 输入坐标除以 `ui_scale`
- [x] T05 `Application` 读取 `COSINE_UI_SCALE`（auto / 数值）并注入三处
- [x] T06 `text_debug` 使用 `glyph_pixel_size()`
- [x] T07 `test/test_font_atlas_dpi.cpp`（3 用例）+ 注册
- [x] T08 回归：ctest 51/51（build + ASAN）；`COSINE_UI_SCALE=2` 冒烟 exit=0 / validation=0
- [ ] T09 阶段计划勾选 M4.6 T8、SPEC 折叠、决策记录

## 完成定义

- 默认 1.0 时行为与改动前完全一致。
- `COSINE_UI_SCALE=2` 时 UI 整体放大、字形清晰、输入命中正确。
