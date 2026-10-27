# 20261027-offscreen-export

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.0：离屏渲染与 PNG 导出

### 现状（改动前）

- `RenderTarget` 只有结构描述（「不拥有 image/view」），`make_swapchain_target()` 是唯一构造。
- 全仓库无离屏 image、无 `vkCmdCopyImageToBuffer`、无 PNG 写盘
  （`grep screenshot|capture|CopyImageToBuffer` 为空，M1 记录的 F11 截图能力已不存在）。
- `transition_image_layout(const RenderTarget&, ...)` 与 `upload_vertex_data()`
  都硬编码写 `command_buffers_[frame_index_]`，无法在单次命令缓冲里复用。

### 改动

1. `Renderer::render_offscreen(drawable, width, height, ui_scale = 1.0f)`：
   - `create_image` + `create_image_view` 建立 target，格式 `B8G8R8A8_SRGB`；
   - `RenderTarget.final_layout = eTransferSrcOptimal`，复用 `begin_rendering` / `end_rendering`
     完成布局转换；
   - `begin_single_time_commands` → `upload_vertex_data(cb)` → push constant（逻辑尺寸 = 导出尺寸）
     → `record_batches` → `copyImageToBuffer`；
   - 读回后做 BGRA → RGBA，返回 RGBA8。
2. `Renderer::write_png(path, width, height, rgba)`：`stb_image_write`
   （`STB_IMAGE_WRITE_IMPLEMENTATION` 定义在 `renderer.cpp`）。
3. 抽出 `record_batches(command_buffer, target, scale)`，帧路径与离屏路径共用；
   `upload_vertex_data` 增加命令缓冲参数。
4. `transition_image_layout(const RenderTarget&, ...)` 增加命令缓冲参数。
5. `ui_anim_showcase` 支持 `COSINE_EXPORT_PNG=<path>`：完成布局后离屏渲染并退出。

### 三个必须记住的坑

1. **字形同步顺序**：`append_to` 期间会按需加载新字号字形（`revision` 变化）。
   离屏路径必须**先 `append_to`、再 `update_font_atlas_texture()`**；
   否则新字号字形不在纹理里，文字整片透明（按钮用预填的 16px 所以看得见，
   `Label` 用 12/13/15/18px 动态加载所以全消失）。
   窗口路径天然满足这个顺序（`draw()` 之后才 `end()`）。
2. **离屏格式必须与管线一致**：管线 `pColorAttachmentFormats[0]` 是 swapchain 的
   `B8G8R8A8_SRGB`；用 `R8G8B8A8_SRGB` 会触发 VUID 并可能通道错乱。
3. **导出尺寸要与 root 一致**：窗口 DPI 会改变 `root` 尺寸（本机 `framebuffer_scale = 1.5`，
   拿到 1266×1582）。导出前显式 `set_size` + `layout()`，否则 UI 坐标与离屏 target 不匹配。

### 验证

- `COSINE_EXPORT_PNG=/tmp/e4.png ./ui_anim_showcase`：exit=0、validation=0，
  产出 1200×800 PNG；三展区、控制行、进度条、时间与事件标签齐全且位置正确。
- `ctest` **51/51**（build 与 ASAN）。
- `ui_gallery` / `ui_anim_showcase` 窗口路径冒烟：exit=0、validation=0，确认重构未回归。

### 边界（本次不做）

- 离屏渲染同步阻塞（`waitIdle` + 单次命令），只用于导出，不进每帧路径。
- 不做帧序列批量导出与视频编码（M6.6）。
- 导出固定 `ui_scale = 1.0`，高 DPI 下不做 2x 超采样。
- 未做离屏结果作为纹理再采样（后续 M6.5 的多纹理槽再做）。
