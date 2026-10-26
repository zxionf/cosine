# 20261026-style-system

<!-- 为什么改、边界与不属于本次的范围 -->

## 样式系统

### 模型

样式分三层，数据与绘制分离：

1. **调色板与度量**（`ui/theme.hpp`）：颜色字段 + 尺寸字段，仍是唯一数据源。
2. **语义解析**（`ui/style.hpp`）：把控件抽象成「样式角色 + 交互状态」，解析成 `BoxStyle`。
3. **绘制辅助**（`graphics/box.hpp`）：圆角矩形、圆角环、内阴影，纯函数追加到 `DrawList`。

控件不再自己拼 `Rectangle` 与颜色分支，只做三件事：求状态 → `Theme::style(role, state)` → 调绘制辅助。

### 角色与状态

```cpp
enum class StyleRole { Surface, Control, Input, Track, Accent, Row, MenuItem, Tooltip };
enum class ControlState { Normal, Hovered, Pressed, Disabled };

struct BoxStyle {
    glm::vec4 fill{0.0f};
    glm::vec4 border{0.0f};
    float border_width = 0.0f;
    float radius = 0.0f;
    glm::vec4 text{1.0f};
    float text_size = 16.0f;
    glm::vec4 focus_ring{1.0f};
};
```

- `Theme::style(StyleRole, ControlState) const` 返回 `BoxStyle`；未显式定义的状态回落到 `Normal`，再回落到调色板默认值。
- 状态优先级固定：`Disabled > Pressed > Hovered > Normal`；`Focused` 不作为填充状态，焦点环是叠加层（可与 hover 共存）。
- `control_state(bool enabled, bool hovered, bool pressed)` 是唯一的派生入口，控件不得再自行写 if 链。
- 角色语义：`Surface` 面板/卡片底，`Control` 按钮类，`Input` 输入类框体，`Track` 滑轨/进度/滚动条槽，`Accent` 选中与强调填充，`Row` 列表行，`MenuItem` 菜单项，`Tooltip` 浮层。
- 现有 `*_bg` 调色板字段保留（`Theme::dark/light` 与既有测试依赖），`style()` 从中取值，二者不并存出第二套真相。

### 圆角

- `graphics::append_rounded_rect(list, rect, radius, color)`：`radius` 先 clamp 到 `[0, min(w,h)/2]`。
- 几何用**九宫格不重叠切分**：中心矩形 + 四条边矩形 + 四角扇形。区域互不重叠，预乘 alpha 混合下不会二次提亮，半透明圆角面板（tooltip/dialog）因此可用。
- 角分段数按半径自适应：`segments = clamp((int)ceil(radius * 0.6f), 2, 6)`。
- `radius < 0.75` 退化为现有 4 顶点矩形，保持顶点数与旧路径一致。
- 不做抗锯齿、不做 SDF：边缘为硬边，与当前矩形/线段一致。改用 SDF 需要给 `Vertex` 增加局部坐标、改 `ui.slang` 与管线顶点描述、波及文本顶点，收益不足以支持现在做（记为后续评估项）。

### 圆角环（边框与焦点环）

- `graphics::append_rounded_ring(list, rect, radius, thickness, color)`：外圆角矩形与内圆角矩形之间的环带，内部不填充、不遮挡内容。
- 边框与焦点环共用该函数：`thickness = style.border_width` 画边框，`thickness = focus_thickness` 画焦点环。
- 焦点环画在控件外扩 `focus_offset`（默认 2px）的矩形上，不再用四条实心矩形压住内容边缘。

### 焦点环与键盘可见性

- `enum class InputMode { Mouse, Keyboard }` 放进 `UIContext`；`UISystem` 维护：`KeyDown`/`KeyUp` 置 `Keyboard`，`MouseButtonDown`/`MouseMove` 置 `Mouse`。
- `Widget::focus_visible()` = 有焦点 且 `input_mode == Keyboard`；鼠标点击聚焦时不画焦点环，键盘 Tab/方向键聚焦时画。
- 焦点由基类持有：`Widget` 增加 `focused_` 与 `set_focused()`，`on_focus_gained/lost` 默认维护它；控件删除自己的 `focused_` 字段，改读 `is_focused()`。`hovered_` 已在基类，沿用。
- `pressed_` 只在少数控件有意义（Button/Checkbox/Dropdown/Radio/Slider/TabBar/MenuItem），保留在控件内部，但要通过 `control_state()` 参与解析。

### Theme 扩展

- 圆角档位：`radius_small = 3`、`radius_medium = 6`、`radius_large = 10`；`radius`（控件默认，取 `radius_medium`）与 `radius_panel`（面板）。
- 胶囊圆角：`radius_pill = 1000`（故意取大，绘制时 clamp 到短边一半）。Slider 轨道/手柄、Progress、Radio 用它，因此「圆形控件」也跟随圆角开关；classic 样式归零后它们是直角。
- 强调色：新增 `accent`（dark `#3399FF` / light `#1F73F2`）。`focus_ring`、`checkbox_fill`、`slider_fill`、进度填充统一由 `accent` 派生，旧字段保留为别名值。
- 密度：`enum class Density { Compact, Normal, Spacious }`，`Theme::with_density(Density)` 返回缩放后的副本（Compact ×0.85、Normal ×1.0、Spacious ×1.15），作用于 `padding`/`spacing`/`control_height`/`row_height`/`checkbox_size`/`slider_handle_radius`/`scrollbar_width`；字号与圆角按 ×0.9 / ×1.0 / ×1.1 缩放。
- 字号档位：`text_size`（Body）、`small_text_size`（Caption）、`heading_text_size`（Heading）；`typography.hpp` 的比例角色保持不变，只把基础档位对齐到这三个字段。
- 内阴影：`inset_shadow`（颜色，alpha 低）+ `inset_shadow_size`（默认 6）。`append_inset_shadow()` 用 3–4 层递减 alpha 的圆角条模拟，只用于 `Surface` 顶部与 `Tooltip` 内缘；classic 样式下 `inset_shadow_size` 归零。
- 角色填充语义：`Row` / `MenuItem` 的 Hover/Pressed 沿用按钮色系（`button_hover` / `button_pressed`），保持旧观感；选中态由控件自己叠加 `accent` 半透明，不进 `BoxStyle`。

### 几何风格与焦点策略（两维正交）

旧视觉不是被替掉，而是两个可自由组合的维度：

- **几何**：`FocusStyle { Outline, Border }`。`Outline` = 外扩圆角环，`Border` = 贴控件内边缘的四条实心框（旧 Button/TextInput 的画法）；`with_rounding(false)` 同时把全部 `radius*`（含 `radius_pill`）归零。
- **时机**：`FocusPolicy { Keyboard, Always, Never }`。`Keyboard` 只在键盘操作期间显示，`Always` 聚焦即显示，`Never` 从不显示；几何与时机互不影响，任意组合都成立。
- `Theme::with_rounding(bool)` 只改几何（圆角与 `focus_style`），不碰 `focus_policy`；`Theme::with_focus_policy(...)` 只改时机。链式调用后写者胜，顺序任意。
- `with_rounding` 是单向变换：要回到圆角样式，从基础主题重新派生（`base.with_rounding(true)`），避免清零值被当成新基准。
- 判定集中在 `Widget::focus_visible()`（读 `focus_policy`）与 `Widget::append_focus_ring()`（读 `focus_style`），控件不分支。
- 像素对齐：Checkbox 的外框、边框环与勾选块都 `round` 到整像素，勾选块 inset 也取整（`round(box*0.25)`），保证四边留白严格相等；半像素 inset 会让勾选块看上去偏心、边缘发虚。
- 经典预设 = `with_rounding(false).with_focus_policy(Always)`，复刻旧观感（鼠标点击聚焦也显示边框）；现代预设 = `with_rounding(true).with_focus_policy(Keyboard)`。
- gallery 左侧导航有 `Style: Rounded/Classic` 与 `Focus: Keyboard/Always/Never` 两个独立按钮；`Style` 分区另有 `Rounded/Classic` 与 `Keyboard only/Always/Never` 两行对照。环境变量：`COSINE_GALLERY_CLASSIC=1`、`COSINE_GALLERY_FOCUS=keyboard|always|never`。
- 切换 Style 会把焦点时机复位为该风格的预设（Classic → Always、Rounded → Keyboard），之后点 Focus 按钮可任意覆盖，两维仍然自由组合。

### 第二批落地细节（列表与浮层）

- `Panel`：`Surface` 角色 + `radius_panel` 圆角 + `append_rounded_ring` 边框，hover 边框仍用 `accent`；顶部加 `append_inset_shadow`。
- `Tooltip` / `MenuPopup` / `DropdownList`：`Surface` 或 `Tooltip` 角色 + 圆角；半透明底（`tooltip_bg` alpha 0.95）依赖九宫格不重叠切分。
- `MenuBar` / `Dropdown` 按钮 / `TabBar` 标签：`Control` 角色（Normal/Hovered/Pressed），焦点统一 `append_focus_ring`。
- `Table` / `TreeView`：面板背景 `Surface`，行高亮 `Row` 角色 + `radius_small`（左右内缩 2px），选中态仍是 `accent` 28% 半透明叠加，表头保持直角（面板内部元素，不跟随圆角）。
- 滚动条（ScrollView / Table / TreeView）：轨道 `Track` 角色，滑块 `scrollbar_thumb`，圆角用 `radius_pill`（modern 圆头 / classic 直角）。
- `Splitter`：手柄颜色仍为 `panel_border`，hover 时用 `Control` hover 填充，圆角 `radius_small`。
- 内阴影随 `with_rounding(false)` 关闭（`inset_shadow_size` 归零），classic 完全回到旧观感。

### 迁移顺序

- 第一批（状态最丰富、复用最广）：`Button`、`Checkbox`、`Radio`、`Slider`、`Progress`、`TextInput`。
- 第二批（列表与浮层）：`Panel`、`MenuBar`、`Dropdown`、`TabBar`、`Table`、`TreeView`、`Tooltip`、`Dialog`、`ScrollView` 滚动条、`Splitter`。
- 每批迁完即构建 + 全量 ctest，不留不可编译的中间态。

### 验证

- `test_ui_style`：`style()` 各角色/状态取值来源；`control_state()` 优先级；`with_density()` 缩放与幂等；`with_rounding()` 归零与 `Border` 焦点框几何（16 顶点、不外扩）；`with_focus_policy()` 与几何的 2×3 组合矩阵（可见性 + 几何顶点数）；圆角 clamp；圆角矩形几何（顶点/索引数、索引不越界、退化到 4 顶点）；环带厚度与半径退化；圆形控件退化（slider 12 / progress 24 / radio 26 顶点）；Checkbox 像素对齐（整数顶点、勾选块与外框同心、四边留白相等）。
- `test_ui_theme` 现有断言必须继续通过（`name`、明暗对比、切主题后 padding/text_size 生效）。
- gallery 新增 `Style` 分区：同一组控件并列展示 normal/hover/pressed/disabled/focused 五种外观（用 `set_hovered()`/`set_enabled(false)`/`set_focused()` 构造伪状态），半径与密度各一组按钮，键盘 Tab 进入控件时可见焦点环、鼠标点击后隐藏。
- 全量：`cmake --build build -j` + `ctest` 全绿，gallery 连续运行无 validation error。

## 边界

- 不做颜色/圆角插值过渡：需要 `Widget::update(dt)` 与 `UISystem::tick(dt)` 的节拍改造，与「UI 动画」事项同批做，避免本次同时改状态模型与时间模型。
- 经典样式只保留「直角 + 贴边焦点框」这一组几何，不保留其它历史特例（各控件曾经各自写的 hover 高亮色改为统一 `ControlState`）。
- 不做 SDF/MSDF 圆角、不做真正的高斯内阴影与投影、不做模糊/毛玻璃。
- 不做主题文件（JSON/TOML）加载与样式表覆盖；`Theme` 仍是 C++ 结构体，`style()` 可被继承替换。
- 不改控件布局语义（度量字段只被密度缩放，不改变 measure/arrange 规则）。
- 不引入第三方 UI/样式库。
