# 20261027-offscreen-export 任务清单

- [x] T01 离屏 target：`create_image` + `create_image_view`，格式对齐管线
- [x] T02 `render_offscreen`：单次命令 + 复用 `begin_rendering`/`end_rendering` + `copyImageToBuffer`
- [x] T03 抽出 `record_batches`，帧路径与离屏路径共用
- [x] T04 `upload_vertex_data` 与 `transition_image_layout(RenderTarget)` 增加命令缓冲参数
- [x] T05 读回 BGRA→RGBA + `write_png`（`stb_image_write`）
- [x] T06 `ui_anim_showcase` 接入 `COSINE_EXPORT_PNG`
- [x] T07 验证：导出 1200×800 PNG、内容正确、validation=0
- [x] T08 回归：ctest 51/51（build + ASAN）；gallery/showcase 窗口冒烟 exit=0、validation=0
- [ ] T09 文档：`docs/examples.md` 补 `COSINE_EXPORT_PNG`；阶段计划勾 M6.0；决策记录；close

## 完成定义

- 任意 `Drawable`（含整棵 UI 树）可离屏渲染为 RGBA8 并写成 PNG。
- 导出的画面与窗口渲染一致；窗口路径无回归。
