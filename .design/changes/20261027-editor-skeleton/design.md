# 20261027-editor-skeleton

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.4a：编辑器骨架与产品位置

### 位置决策

- `src/cosine/` 放**产品层**：`editor.hpp/.cpp`（静态库 `cosine_editor`）+ `main.cpp`（可执行 `cosine`）。
- **`Application` 不搬**：仍在 `src/xel_application.*`（`cosine_app` 库），四个 example 继续使用。
  搬迁会连带改 4 个 example 的 include 与 CMake 源列表，收益低。
- `examples/` 保持「演示」语义；打包只打 `cosine`（脚本改动留到 M6.5）。
- `option(COSINE_BUILD_EDITOR ON)`：只想构建库/测试时可关。

### 外观默认

- **classic（默认）**：`Theme::dark().with_rounding(false).with_radius_scale(0).with_transition_ms(0)`。
- **圆角现代样式为实验性选项**：View → Experimental rounded style，或工具栏 `Style` 按钮。
  切换后 `with_rounding(true).with_radius_scale(1).with_transition_ms(120)`。
  注意 `with_rounding` 是单向变换，每次都从 `Theme::dark()` 重新派生。
- **滚动默认 `Smooth`**：`ScrollView` / `TreeView` / `Table` 的成员默认值已是 `ScrollMode::Smooth`，
  编辑器不额外覆盖。

### 骨架内容

- 菜单：File（New/Open/Save/Exit，占位）、Edit（Undo/Redo，占位）、View（实验性圆角）、
  Playback（Play/Reset，占位）。
- 工具栏：`Play` / `Reset` / `Add Shape` / `Style`。其中 **`Add Shape` 是唯一真实动作**：
  往 `Project` 加一个 Shape 节点并刷新场景树。
- 面板：左 `TreeView`（绑 `Project` 真实层级）、中 `SceneView`、右属性占位（只占工作区高度）、
  底部**全宽**时间轴占位、状态栏。窄窗口下压缩两侧栏，优先保证预览宽度。
- `Editor::tick` 跟踪 root 尺寸变化并重排；固定像素分区 + 窗口自适应。

### 顺带修复

`Label::invalidate_measure()` 与 `Widget::invalidate_measure()` 虚函数同名（不同签名），
触发 `-Woverloaded-virtual`。改名为 `invalidate_measure_cache()`。

### 验证

- `COSINE_EXIT_AFTER=3 ./build/src/cosine/cosine`：exit=0、validation=0。
- 截图：菜单 / 工具栏 / 场景树（root→Stage→Card/Title）/ 预览（"Hello cosine" + 卡片）/
  属性占位 / 时间轴占位 / 状态栏齐全；classic 直角。
- `ctest` **53/53**。
- 构建仅剩 `button.hpp` 的既有 `-Wunused-variable`（非本次引入）。

### 边界（本次不做）

- 不做 `Splitter`（固定分区）；不做选中联动（树 ↔ 属性 ↔ 时间轴）；不接命令栈与 Player。
- `File/Edit/Playback` 菜单项均为占位，点击只改状态栏文本。
- 未设窗口尺寸下限：极窄时面板会被挤压，M6.4b 处理。
