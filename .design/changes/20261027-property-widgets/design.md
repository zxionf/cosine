# 20261027-property-widgets

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.2 第一阶段：属性编辑控件与 2D 旋转

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `DragFloat`：拖动改值 + 键盘微调 + clamp/step/精度/后缀/禁用态 |
| `src/ui/scene_view.hpp` | 2D `rotation`：顶点绕节点中心旋转，父级旋转累积 |
| `src/cosine/editor.*` | 属性面板 11 行（Pos/Rotation/Scale/Size/Color），选中联动 |
| `test/test_ui_drag_float.cpp` | 6 组用例 |
| `src/cosine/editor.cpp` | `FocusPolicy::Always`（按你的要求） |

### 关键实现

1. **`SceneView` 旋转**：`draw_node` 增加 `parent_rotation`，子节点偏移先按父旋转再累加；
   自身 `rotation = parent_rotation + node->transform.rotation`。
   - `|rotation| < 1e-4` 仍走轴对齐 `Rectangle`（保持像素锐利）；
   - 否则用 `append_rotated_rect`：绕节点中心算 4 角，两个三角形，预乘 alpha 与 `Rectangle` 一致。
   - **椭圆与文字暂不旋转**（位图圆角不支持，文字按旋转后的锚点定位）。
2. **`DragFloat`**：`on_mouse_down/move/up` 记录起点做差值；灵敏度默认「200px 走完全程」，
   可用 `set_step` 覆盖；键盘 Left/Right/Up/Down/Home/End 微调；`set_enabled(false)` 全灰并忽略输入。
3. **属性面板**：11 行（Pos X/Y、Rotation(deg)、Scale X/Y、Size W/H、Color RGBA），
   回调直接改 `Node` 字段并 `mark_layout_dirty()`；选中通过场景树 path → `NodeId` 映射。
4. **FocusPolicy::Always**：焦点环不再依赖「上次输入是否键盘」。

### 踩到的坑

1. **`scene` 命名空间是 `xel::scene`，不是 `xel::anim::scene`**。`anim` 里只有 `Scene`/`Timeline` 等类型。
   `editor.cpp` 在本 TU 内用 `namespace xel::anim { namespace scene = ::xel::scene; }` 别名兼容。
2. **`std::format` 嵌套占位符**：`"{:.{}}f{}"` 会把 `f` 当字面量、精度作用于默认格式，
   输出 `1e+02` / `1f`。正确写法 `"{:.{}f}{}"`。
3. `Widget` 已有 `virtual void invalidate_measure()`；`Label` 的同名 `const` 版本会隐藏它
   （M6.4a 已改名 `invalidate_measure_cache()`，此处仅记录）。

### 验证

- `test_ui_drag_float`：clamp、拖动改值（含灵敏度）、显式 step、键盘、禁用、失焦停止拖动。
- `ctest` **54/54**。
- 编辑器截图：选中 Card 后属性面板显示 `120.0 / 150.0 / 0.0deg / 1.00 / 180.0 / 110.0 / 0.95…`；
  `COSINE_EXIT_AFTER=3` exit=0、validation=0。

### 边界（本次未做）

- **色板 + 十六进制颜色输入**：当前颜色用 4 个 `DragFloat` 分量，下一阶段补 `ColorInput`。
- **`SpinBox`（文本精确输入）**：需要与 `TextInput` 组合，随属性面板二次打磨再做。
- **gallery「属性编辑」分区**：待补，用于脱离编辑器单独验收。
- `SceneView` 旋转的单元测试：暂由编辑器截图人工验收。
