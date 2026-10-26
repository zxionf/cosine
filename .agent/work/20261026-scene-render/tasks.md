# 20261026-scene-render 任务清单

- [x] T01 `ui/scene_view.hpp`：SceneView（scene 指针、measure/arrange、append_to）
- [x] T02 递归绘制：position/scale 累积、隐藏子树跳过、Shape/Text/Image 映射
- [x] T03 `cosine_ui` PUBLIC 链接 `cosine_anim`
- [x] T04 `ui_anim_demo` 改为 Scene + Timeline + SceneView 驱动
- [x] T05 `test/test_ui_scene_view.cpp` + 注册
- [x] T06 回归：ctest 46/46（build + ASAN）、demo 正常退出 `validation=0`、截图确认
- [ ] T07 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- Scene 可被渲染到 DrawList，变换累积与可见性正确。
- demo 由 Timeline 驱动，可播放/暂停/重置，视觉与数据一致。
- 全量测试与 ASAN 通过，退出无校验错误。
