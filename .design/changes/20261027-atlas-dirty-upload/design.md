# 20261027-atlas-dirty-upload

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.6 T2：纹理上传优化（脏矩形）

### 现状

`update_font_atlas_texture()` 在 `revision` 变化时：分配整张 staging（2048²×4 = 16MB）、
`memcpy` 整张、整张 `vkCmdCopyBufferToImage`。动态加载**一个**字形就要传 16MB。

### 改动

1. `FontAtlas` 记录脏矩形：`add_glyph` 成功后扩展 `dirty_min/max_x/y`（含端点）；
   `add_empty_glyph` 不产生脏区；`load()` / `begin_generated()` 清空。
2. 新增 `take_dirty_rect(x, y, w, h)`：读后清零，无脏区返回 `false`。
3. `Renderer::update_font_atlas_texture()`：
   - 只按脏矩形分配 staging，逐行 `memcpy`；
   - `VkBufferImageCopy` 用 `imageOffset = {dx, dy}`、`imageExtent = {w, h, 1}`、
     `bufferRowLength = 0`（紧凑布局）；
   - 只新增无像素字形时直接返回，不触发上传。

### 验证

- `test_font_atlas_space.cpp` 新增 2 个用例：脏矩形追踪 / 合并 / 清零、空白字形不产生脏区。
- `ctest` **48/48**（build 与 ASAN）。
- gallery 冒烟：`COSINE_EXIT_AFTER=3` exit=0、validation=0；截图多档字号文字清晰、无错位。

### 效果

单个字形的上传量从 16MB 降到该字形所在矩形（通常几十 KB）。

### 边界（本次不做）

- 仍为一次性 staging 分配（未做持久 staging 环）；脏区通常很小，分配开销可忽略。
- 多个字形分散时会合并成一个包围盒（可能偏大），未做多矩形合并。
- 不做分页图集 / 多纹理槽。
