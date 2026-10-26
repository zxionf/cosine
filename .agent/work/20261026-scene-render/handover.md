# 20261026-scene-render 交接

## 结果

- 新增 `src/ui/scene_view.hpp`（header-only Widget）：
  - 递归绘制 Scene，累积父级 `position` / `scale`；
  - `visible == false` 的节点**连同子树**跳过；
  - Shape（矩形/椭圆）、Text、Image（占位色块）；Group/Camera 只传递变换；
  - `rotation` 暂不参与。
- `cosine_ui` PUBLIC 链接 `cosine_anim`（anim 仍只依赖 glm）。
- `ui_anim_demo` 改为 Scene + Timeline + SceneView：
  - 5 个 Shape + 5 个 Text + 参考线；
  - 每帧 `timeline.advance(dt)` → `timeline.apply(scene)` → 渲染；
  - Pause/Play/Reset 直接操作 Timeline。
- 新增 `test/test_ui_scene_view.cpp`（5 个用例），注册为 `ui_scene_view`。

## 验证

- `ctest` 46/46（build 与 ASAN）。
- `COSINE_EXIT_AFTER=2 ./ui_anim_demo`：exit=0、`validation` 计数 0。
- 截图确认：五条轨道方块、参考线、控制行、进度与时间标签全部由 scene 渲染。
- `sele close` 为 `appended`（`DECISIONS #118`）。

## 注意

- `Demo` 必须先于 `build()` 存在：`SceneView` 持有 `&demo.scene`，按值返回会悬垂。
- `SceneView` 不缓存节点位置，scene 变化后无需 `mark_layout_dirty()`，`append_to` 每帧重放。
- 绘制顺序 = `children` 顺序（无 z 排序）。

## 下一步（M4 收尾）

1. **player**：独立类封装 `Timeline + Scene`，提供 `tick(dt)` / `seek(t)` / `render_time()`，
   便于「逐帧确定性渲染」与 UI 拖动进度。
2. **events**：时间点回调（进入/离开区间、动画结束）。
3. rotation / Camera 变换、Image 真实纹理（M6）。
