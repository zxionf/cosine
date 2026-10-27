# 20261027-property-widgets 交接

## 结果（M6.2 第一阶段）

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `DragFloat`：拖动 + 键盘 + clamp/step/精度/后缀/禁用 |
| `src/ui/scene_view.hpp` | 2D `rotation`：绕节点中心、父旋转累积、轴对齐快路径 |
| `src/cosine/editor.*` | 属性面板 11 行 + 场景树选中联动 + `FocusPolicy::Always` |
| `test/test_ui_drag_float.cpp` | 6 组用例 |

## 三个坑

1. **`scene` 命名空间是 `xel::scene`**，不是 `xel::anim::scene`（`anim` 里只有 `Scene`/`Timeline` 类型）。
   `editor.cpp` 用 TU 内别名 `namespace xel::anim { namespace scene = ::xel::scene; }` 兼容。
2. **`std::format` 嵌套占位符**：`"{:.{}}f{}"` 会让 `f` 变字面量、精度作用在默认格式上，
   显示 `1e+02` / `1f`。正确是 `"{:.{}f}{}"`。
3. **`sele close` 之后 `STATE.md` 的 `Active Change` 可能残留 change id**（本次 close 报
   "Active Change untouched"），记得手动改回「无」。

## 验证

- `test_ui_drag_float`：clamp、拖动改值、显式 step、键盘、禁用、失焦停止拖动。
- `ctest` **54/54**。
- 编辑器截图：选中 Card 后属性面板显示 `120.0 / 150.0 / 0.0deg / 1.00 / 180.0 / 110.0 / 0.95…`；
  `COSINE_EXIT_AFTER=3` exit=0、validation=0。

## 第二阶段（未做）

1. `ColorInput`：色板（内嵌若干色块）+ 十六进制输入，替换 4 个颜色 `DragFloat`。
2. gallery 增「属性编辑」分区，脱离编辑器独立验收控件。
3. `SceneView` rotation 的单元测试（当前靠截图人工验收）。
4. `SpinBox`（文本精确输入）随属性面板二次打磨再做。

## 下一步

先补第二阶段再进 M6.3 时间轴视图；或按你的意思直接进 M6.3（颜色用分量行也能用）。
