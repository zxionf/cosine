# 20261026-scene-render

<!-- 为什么改、边界与不属于本次的范围 -->

## SceneView：把场景渲染成 DrawList

### 为什么

`scene::Scene` 目前是纯数据，没有渲染路径。M4 的验收是「播放、暂停、拖动、逐帧确定性 render」，
没有渲染就无法验证。先做一个 `ui::SceneView`，把 Scene 摊平成 DrawList 指令，
复用现有 2D 管线与字体图集。

### 设计

`src/ui/scene_view.hpp`（header-only Widget）：

- `SceneView(scene::Scene*)` / `set_scene()` / `scene()`。
- `append_to()`：从 `scene.root()` 递归，累积父级 `position` 与 `scale`；
  `visible == false` 的节点**连同子树**整体跳过。
- 类型映射：
  - `Group` / `Camera`：只传递变换；
  - `Shape`：`ellipse` 时用圆角矩形（半径 = 短边一半），否则 `graphics::Rectangle`；
  - `Text`：`FontAtlas::append_text`，裁剪在控件内；
  - `Image`：占位色块（真实纹理等 M6）。
- `rotation` 暂不参与（M4 后续），位置与缩放已支持。
- 依赖方向：`cosine_ui` PUBLIC 链接 `cosine_anim`（`anim` 仍只依赖 glm，不反向依赖 UI）。

### demo

`ui_anim_demo` 改为数据驱动：

- Scene 内 5 个 `Shape` 节点（五种缓动的方块）+ 5 个 `Text` 节点（轨道名）+ 参考线；
- `anim::Timeline` 为每个方块添加 position 轨道，周期 3s 循环；
- 每帧 `timeline.advance(dt)` → `timeline.apply(scene)` → UI 渲染 `SceneView`；
- Pause/Play、Reset 直接操作 Timeline；进度条与时间标签读 `timeline.time()`。

注意 `Demo` 必须先于 `build()` 存在（`SceneView` 持有 `&demo.scene`）。

### 验证

新建 `test/test_ui_scene_view.cpp` 并注册：

- Shape 产生顶点；空场景为 0。
- 隐藏父节点 → 子树整体不绘制（顶点数为 0）。
- 父 `position` 与 `scale` 累积：子节点首顶点坐标、矩形宽度符合预期。
- demo：`COSINE_EXIT_AFTER=2 ./ui_anim_demo` → exit=0、`validation` 计数 0；截图确认渲染。

### 边界（本次不做）

- 不处理 `rotation`、Camera 变换、z 排序（按 children 顺序绘制）。
- Image 真实纹理（M6）、Text 的换行/对齐（当前左对齐单行）。
- 不做脏矩形/局部重绘，SceneView 每帧重放。
