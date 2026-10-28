# 20261028-texture-pipeline

<!-- 为什么改、边界与不属于本次的范围 -->

## 图片链路：Sprite 图元 + 节点资源路径 + 编辑器导入

### 接上一批

多槽基础设施（`20261028-texture-slots`）已经让 16 个槽可用，但没有任何绘制路径。
这批把「导入 png → 显示在节点上 → 存盘 → 重载仍指向同一资源」的第一段闭环打通。

### 三处语义决定

1. **`texture_id` = 图片槽索引 + 1，0 = 未绑定**
   保持「0 表示没有」的旧语义，旧文件读回行为不变；换槽号时不需要额外哨兵值。

2. **`texture_path` 是稳定标识，`texture_id` 只是运行时句柄**
   句柄随进程、上传顺序变化；只有路径能跨进程保持一致。序列化只保证路径，
   读回后由 `rebind_textures()` 重新上传并把句柄补回节点。

3. **未绑定纹理的 Image 节点回退成**色块**，不显示占位棋盘格**
   占位棋盘格留给「槽位空着」的 GPU 资源层；节点层面若显示棋盘格，
   会让人误以为图片加载失败。旧文件里的 Image 节点读回后应保持原样（色块）。

### 实现

- `graphics::Sprite`（`src/graphics/sprite.hpp`）：带 UV + 槽号的矩形图元，
  沿用 `Rectangle` 的预乘 alpha 约定。
- `SceneView`：新增 `append_rotated_quad`（槽 + UV 可指定的旋转四边形），
  `append_rotated_rect` 改为它的薄封装；`Image` 分支按 `texture_id` 选择 Sprite
  或色块，旋转时走 `append_rotated_quad`（图片也能转）。
- `scene::Node`：新增 `texture_path`。
- 序列化：`kFormatMinor` 0 → 1；节点表在 `text` 之后写 `texture_path`；
  读 1.0 文件时跳过该字段（`minor >= 1` 才读）。`docs/project_format.md` 补节点字段
  与版本变更表。
- `TextureCache`（`src/cosine/texture_cache.{hpp,cpp}`）：路径 → 槽，去重；
  槽满（14）返回 0；`clear()` 只清映射，GPU 资源由 renderer 持有。
- 编辑器：构造增加 `Renderer&`；`import_image()` 上传并把选中节点（无选中时用 demo 卡片）
  切成 Image 类型；`load_project()` 末尾调 `rebind_textures()` 重建所有路径 →
  句柄的映射。`COSINE_EDITOR_IMAGE=<png>` 作为自测入口。

### 验证

- `ctest` **59/59**（`test_anim_serialize` 的样例新增一个带路径的 Image 节点，
  断言 `texture_path` 与 `texture_id` 原样回来；节点数、children 顺序断言同步更新）。
- 编辑器冒烟 exit=0 / validation=0。
- `COSINE_EDITOR_IMAGE=/tmp/lab_img.png COSINE_EXPORT_PNG=...` 导出：卡片节点显示渐变图，
  状态栏显示 `imported /tmp/lab_img.png`。

### 边界

- 只做 **Fill**（拉伸填满）。`Contain` / `Cover` 需要图片宽高比，节点上还没有该字段。
- 槽满即失败返回，没有 LRU 淘汰；`rebind_textures` 每次重载都重新上传全部图片。
- 1.0 文件兼容分支（`minor >= 1`）没有单独的单测，只保证代码路径存在。
- `upload_texture` 在 UI 回调里同步执行（内部 `waitIdle`），批量导入需要改成帧间队列。
