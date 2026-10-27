# 20261027-atlas-dirty-upload 任务清单

- [x] T01 `FontAtlas` 脏矩形字段 + `add_glyph` 更新 + `load`/`begin_generated` 清空
- [x] T02 `take_dirty_rect(x, y, w, h)`（读后清零）
- [x] T03 `Renderer::update_font_atlas_texture` 改为区域上传（紧凑 buffer + `imageOffset/imageExtent`）
- [x] T04 只新增空白字形时跳过上传
- [x] T05 测试：脏矩形追踪/合并/清零、空白字形无脏区
- [x] T06 回归：ctest 48/48（build + ASAN）；gallery 冒烟 exit=0 / validation=0
- [ ] T07 阶段计划勾选 M4.6 T2、SPEC 折叠、决策记录

## 完成定义

- 新增字形不再重传整张 2048²，改为只传脏矩形。
- 文字渲染正确，无错位/花屏。
