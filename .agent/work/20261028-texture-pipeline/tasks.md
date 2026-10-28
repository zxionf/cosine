# 20261028-texture-pipeline 任务清单

- [x] T01 `graphics::Sprite`（UV + 槽号，预乘 alpha）
- [x] T02 `SceneView`：`append_rotated_quad` + `Image` 分支（Sprite / 色块回退 / 旋转）
- [x] T03 `scene::Node::texture_path`，`texture_id` 语义定为「槽索引 + 1」
- [x] T04 序列化 1.1：写 `texture_path`，读 1.0 时跳过
- [x] T05 `TextureCache`（路径去重、槽满返回 0）
- [x] T06 编辑器：`import_image` / `rebind_textures` / `Editor` 持 `Renderer&`
- [x] T07 `COSINE_EDITOR_IMAGE` 自测入口
- [x] T08 测试：round-trip 保留路径与句柄；ctest 59/59
- [x] T09 冒烟 exit=0 / validation=0；导出确认图片显示在节点上
- [x] T10 `docs/project_format.md` 补字段与 1.1 变更表
- [ ] T11 决策记录、SPEC 折叠、close

## 完成定义

- `COSINE_EDITOR_IMAGE=<png>` 能把图片显示在节点上（已验证）。
- 保存的项目里存的是路径，重载后能重新绑定（路径 round-trip 有测试覆盖）。
