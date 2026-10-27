# 20261027-atlas-dirty-upload 交接

## 结果

- `graphics/FontAtlas`：
  - 新增脏矩形字段（`dirty_min_x_/y_`、`dirty_max_x_/y_`）；
  - `add_glyph` 成功后扩展包围盒；`add_empty_glyph` 不产生脏区；
  - `load()` / `begin_generated()` 清空脏区；
  - 新增 `take_dirty_rect(x, y, w, h)`：读后清零，无脏区返回 `false`。
- `Renderer::update_font_atlas_texture()`：
  - 按脏矩形分配 staging，逐行 `memcpy`；
  - `VkBufferImageCopy` 用 `imageOffset = {dx, dy}`、`imageExtent = {w, h, 1}`、
    `bufferRowLength = 0`（紧凑）；
  - 只有空白字形时直接返回，不触发上传。

## 验证

- `ctest` **48/48**（build 与 ASAN）。
- 新增 2 个用例：脏矩形追踪/合并/清零、空白字形无脏区。
- gallery 冒烟 `COSINE_EXIT_AFTER=3`：exit=0、validation=0；截图多档字号文字清晰无错位。
- `sele close` 为 `appended`（`DECISIONS #122`）；阶段计划 M4.6 的 T2 已勾选。

## 效果

单个字形上传量：**16MB → 几十 KB**（该字形所在矩形）。

## 注意与后续

- 脏区是**单个包围盒**：若一帧内加载的多个字形分散在图集各处，包围盒会偏大。
  若将来成为问题，可改成「按行合并」或「多矩形列表 + 多次 `vkCmdCopyBufferToImage`」。
- 每次上传仍新建 staging buffer。图集初始化走的是 `create_font_texture()` 的整张路径，
  不受影响。
- 分页图集 / 多纹理槽仍未做（M6 资源管线再议）。

## 下一步

- **M4.6 其余项**（T3 测量缓存 / 行高语义 / 字距排版等），或切回 **M5 XEL 绑定**。
