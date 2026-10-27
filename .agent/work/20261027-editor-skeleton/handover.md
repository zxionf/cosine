# 20261027-editor-skeleton 交接

## 结果

| 文件 | 内容 |
|---|---|
| `src/cosine/editor.hpp/.cpp` | `Editor`：菜单、工具栏、三栏面板、排版、主题 |
| `src/cosine/main.cpp` | `Application` + `Editor` + 主循环（含 `COSINE_EXIT_AFTER`） |
| `src/cosine/CMakeLists.txt` | 库 `cosine_editor` + 可执行 `cosine` |
| `src/CMakeLists.txt` | `option(COSINE_BUILD_EDITOR ON)` + `add_subdirectory(cosine)` |

运行：

```bash
./build/src/cosine/cosine
COSINE_EXIT_AFTER=3 ./build/src/cosine/cosine   # 冒烟
```

## 布局（1280×840）

```
菜单 1280×26
工具栏 1280×40
├ 场景树 240×492 │ 预览 740×492 │ 属性 300×492   ← 属性只占工作区高
时间轴 1280×260（全宽）
状态栏 1280×22
```

- 时间轴横跨全宽：2D 关键帧编辑需要横向空间。
- 属性面板不再贯穿到底，收在工作区内。
- 窄窗口下按比例压缩场景树/属性栏，保证预览 ≥ 200px。
- M6.4b 用 `Splitter` 替换固定像素分区时，这套比例就是初始 ratio。

## 外观默认（按你的要求）

- **classic**：`Theme::dark().with_rounding(false).with_radius_scale(0).with_transition_ms(0)`
- **圆角现代样式 = 实验性选项**：`View → Experimental rounded style`，或工具栏 `Style` 按钮
  → `with_rounding(true).with_radius_scale(1).with_transition_ms(120)`
- **滚动默认 `Smooth`**：三个滚动控件的成员默认值本来就是 Smooth，编辑器不覆盖

## 三个坑

1. **`Label::invalidate_measure` 与 `Widget::invalidate_measure()` 同名**（签名不同）→
   `-Woverloaded-virtual`。已改名 `invalidate_measure_cache()`。
2. **`std::function` 不能在自己的初始化列表里捕获自己**：
   `std::function<T> f = [&f](...){...};` 是未定义行为。要写成「先声明、再赋值」。
3. **`UIRoot` 是自由画布**：`layout_child` 用 `child.x()/y()` + `measure()`，
   所以 `set_pos` + `set_min/max_size` + `set_size` 三件套要一起设，否则尺寸被 clamp 回旧值
   （`TreeView` 构造时锁了 min/max，必须重设）。

## 验证

- `COSINE_EXIT_AFTER=3 ./build/src/cosine/cosine`：exit=0、validation=0。
- 截图：菜单 / 工具栏 / 场景树（root→Stage→Card/Title）/ 预览（"Hello cosine" + 卡片）/
  属性占位 / 时间轴占位 / 状态栏齐全，classic 直角。
- `ctest` **53/53**。
- 构建仅剩 `button.hpp` 的既有 `-Wunused-variable`（非本次引入，可顺手清）。

## 下一步：M6.2 属性编辑控件与 2D 旋转

- `DragFloat` / `SpinBox`（float 与 vec2/vec3/vec4 分量）、色板 + 十六进制颜色输入。
- `SceneView` 2D `rotation`（顶点绕节点中心旋转，不上 mat4）。
- 面板接口按 `Editor` 里预留的 `property_panel_` 位置替换占位 Label。
- 控件改动会动到 `docs/examples.md`（gallery 分区）与 `docs/index.md`（控件清单），记得同步。
