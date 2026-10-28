# 20261028-texture-slots

<!-- 为什么改、边界与不属于本次的范围 -->

## 多纹理槽：0=White、1=字体图集、2–15=图片

### 背景

CO-2 要让编辑器显示导入的 png。动手前先弄清现有槽的用途：`TextureSlot::Texture`
（槽 1）装的**是字体图集**（`create_texture_image` 构建并上传 CJK 图集），
不是「通用纹理」。所以图片必须从槽 2 开始，槽 1 不能动。

### 两条路线的选择

| 方案 | 做法 | 代价 |
|---|---|---|
| 多 descriptor set + 多 draw | 每张图一个 set，切批时换绑定 | draw call 随纹理数线性增长 |
| 改顶点格式 + 纹理数组 | 顶点带层号，一次画完 | 顶点变宽；`sampler2DArray` 要求同尺寸，与「用户导入任意 png」冲突 |

选前者：**不改顶点格式、不改 shader、不改管线**，与现有 2 槽机制同源。
编辑器预览里图片数量是个位数，多几次 draw 完全不可感。

### 实现

- `drawable.hpp`：`kMaxTextureSlots = 16`、`kFirstImageSlot = 2`、`kMaxImageSlots = 14`、
  `image_slot(index)`、`is_image_slot(slot)`；`DrawBatch::texture_slot` 与
  `DrawList` 的槽状态从 `TextureSlot` 改成 `uint32_t`（保留 `set_texture(TextureSlot)`
  重载，调用点不动）。
- `Renderer`：
  - descriptor pool 与 sets 从 `帧数 × 2` 扩到 `帧数 × 16`；
  - `create_descriptor_sets` 循环所有槽：0 = white、1 = 字体图集、2..15 = 占位视图；
  - `create_placeholder_texture`：8×8 棋盘格，未加载的槽显示它（与「纯白」区分）；
  - `upload_texture(index, w, h, rgba)`：按需重建该槽 image/view（尺寸可变）并把
    所有帧的该槽 descriptor set 写回；失败返回 false；`clear_texture` 恢复占位；
  - `record_batches` 的 set 索引改成 `frame × kMaxTextureSlots + slot`。

### 验证

- `ctest` **59/59**。
- `ui_gallery` / `ui_anim_showcase` / `ui_lab` / `cosine` 四个程序冒烟：exit=0、validation=0。
- **关键回归检查**：`ui_lab` 导出图与改造前 `compare -metric AE` = **0**（逐像素一致），
  说明槽索引、字体槽绑定、纯色槽都没有被破坏。

### 边界

- 槽上限 14 张同时在屏；超出需要第二段的槽分配与淘汰策略。
- 纹理格式固定 `R8G8B8A8Srgb`，与字体图集一致；暂不支持线性空间或其它格式。
- `upload_texture` 在 UI 回调里同步上传（内部 `waitIdle`），低频操作可接受；
  若将来导入批量图片，需要改成帧间队列。
- 本次**没有**绘制路径：`Sprite` 图元、`SceneView` 的 Image 分支、节点资源路径与序列化
  都还没做，属于 CO-2 第一段的后续步骤。
