# 20261028-texture-slots 任务清单

- [x] T01 槽模型常量（`kMaxTextureSlots` / `kFirstImageSlot` / `kMaxImageSlots` / `image_slot`）
- [x] T02 `DrawBatch::texture_slot` 与 `DrawList` 槽状态改 `uint32_t`，保留 `TextureSlot` 重载
- [x] T03 descriptor pool / sets 从 `帧数 × 2` 扩到 `帧数 × 16`
- [x] T04 占位棋盘格纹理 + 未加载槽绑定占位视图
- [x] T05 `Renderer::upload_texture` / `clear_texture` / `update_image_slot_descriptors`
- [x] T06 `record_batches` 用新步长绑定 descriptor set
- [x] T07 修复两处测试里 `batch.texture_slot` 与 `TextureSlot` 的类型比较
- [x] T08 回归：ctest 59/59、四个程序冒烟 val=0、导出图与改造前逐像素一致
- [ ] T09 决策记录、SPEC 折叠、close

## 完成定义

- 槽 0/1 行为与改造前完全一致（导出图像素零差异）。
- 图片槽可以被上传、清空，且不会影响字体与纯色图元。
