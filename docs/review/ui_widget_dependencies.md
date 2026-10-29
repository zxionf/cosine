# src/ui 控件依赖关系清单

## 0. 范围与方法

- 统计对象：`src/ui/` 下 51 个 `.hpp` 与 7 个 `.cpp` 中声明的控件类（含 `detail` 命名空间内部类）。
- 数据来源：逐个文件读取 `#include` 指令，并在剥离注释与字符串字面量后按类名做整词匹配，提取继承、成员/局部构造、`dynamic_cast`、参数与返回值位置的类型引用。
- 只记录依赖事实，不含任何评价、审计或改进建议。
- 依赖边定义：
  - **I（include）**：编译期头文件包含。
  - **H（has-a）**：以成员、`unique_ptr`、内部构造的方式持有另一控件。
  - **is-a**：公有继承另一控件。
  - **U（use）**：仅在参数、返回值、`dynamic_cast`、枚举/样式角色等位置引用类型名，不持有实例。

---

## 1. 非控件基础件（被控件依赖的底座）

| 文件 | 主要导出 | 被以下控件文件 include |
| --- | --- | --- |
| `widget.hpp` | `Widget`（基类，继承外部 `graphics::Drawable`） | 35 个文件直接包含；未直接包含的 23 个：align、debug_overlay、dialog、grid.cpp、hbox.cpp、list、panel、radio_group、rect、root.cpp、scroll_mode、scroll_panel、scroll_state、scroll_view.cpp、style、theme、typography、ui_system.cpp、ui_system.hpp、uicontext、vbox.cpp、widget.hpp 自身、wrap.cpp |
| `theme.hpp` | `Theme`、`BoxStyle`、`StyleRole`、`ControlState` | 30 个文件：button、checkbox、collapsible_header、color_input、dialog、drag_float、dropdown、file_browser、label、menu_bar、panel、preview_ruler、progress、radio、radio_group、scroll_view、section、slider、splitter、style、tab_bar、tab_view、table、text_input、timeline_view、tooltip、tree_view、typography、uicontext、window |
| `style.hpp` | `visual_style()` 等样式取值函数 | 21 个文件：button、checkbox、color_input、drag_float、dropdown、menu_bar、panel、preview_ruler、progress、radio、scroll_view.cpp、slider、splitter、tab_bar、table、text_input、timeline_view、tooltip、tree_view、widget、window |
| `uicontext.hpp` | `UIContext` | `widget.hpp` |
| `rect.hpp` | `Rect` | `widget.hpp` |
| `align.hpp` | `Align` | `widget.hpp`、`grid.hpp`、`wrap.hpp` |
| `scroll_mode.hpp` | `ScrollMode` | `scroll_view.hpp`、`tree_view.hpp`、`scroll_state.hpp` |
| `scroll_state.hpp` | `ScrollState`（滚动数值模型） | `list_view.hpp` |

依赖链：`rect.hpp` / `align.hpp` / `uicontext.hpp` → `widget.hpp`；`theme.hpp` → `style.hpp` → 各控件。

---

## 2. 控件全量清单与基类

| 文件 | 类 | 基类 | 类别 |
| --- | --- | --- | --- |
| `widget.hpp` | `Widget` | `graphics::Drawable`（外部） | 根 |
| `vbox.hpp` | `Vbox` | `Widget` | 布局容器 |
| `hbox.hpp` | `Hbox` | `Widget` | 布局容器 |
| `grid.hpp` | `Grid` | `Widget` | 布局容器 |
| `wrap.hpp` | `Wrap` | `Widget` | 布局容器 |
| `spacer.hpp` | `Spacer` | `Widget` | 占位 |
| `root.hpp` | `UIRoot` | `Widget` | 根画布 / overlay 与模态栈 |
| `panel.hpp` | `Panel` | `Vbox` | 容器 |
| `scroll_view.hpp` | `ScrollView` | `Widget` | 容器 |
| `scroll_panel.hpp` | `ScrollPanel` | `Panel` | 复合容器 |
| `list.hpp` | `List` | `ScrollView` | 复合容器 |
| `section.hpp` | `Section` | `Widget` | 复合容器 |
| `tab_bar.hpp` | `TabBar` | `Widget` | 叶子/条状控件 |
| `tab_view.hpp` | `TabView` | `Widget` | 复合容器 |
| `window.hpp` | `Window` | `Widget` | 复合容器 |
| `dialog.hpp` | `Dialog` | `Panel` | 复合容器 |
| `dialog.hpp` | `DialogHost` | `Widget` | 模态宿主 |
| `scrim.hpp` | `Scrim` | `Widget` | 遮罩 |
| `radio_group.hpp` | `RadioGroup` | `Vbox` | 组合控件 |
| `typography.hpp` | `Heading` / `Paragraph` | `Label` | 文本派生 |
| `label.hpp` | `Label` | `Widget` | 叶子 |
| `button.hpp` | `Button` | `Widget` | 叶子 |
| `checkbox.hpp` | `Checkbox` | `Widget` | 叶子 |
| `radio.hpp` | `Radio` | `Widget` | 叶子 |
| `slider.hpp` | `Slider` | `Widget` | 叶子 |
| `drag_float.hpp` | `DragFloat` | `Widget` | 叶子 |
| `color_input.hpp` | `ColorInput` | `Widget` | 叶子 |
| `text_input.hpp` | `TextInput` | `Widget` | 叶子 |
| `progress.hpp` | `Progress` | `Widget` | 叶子 |
| `image.hpp` | `Image` | `Widget` | 叶子 |
| `color_rect.hpp` | `ColorRect` | `Widget` | 叶子 |
| `splitter.hpp` | `Splitter` | `Widget` | 叶子 |
| `tooltip.hpp` | `Tooltip` | `Widget` | 浮层 |
| `collapsible_header.hpp` | `CollapsibleHeader` | `Widget` | 叶子/条状控件 |
| `preview_ruler.hpp` | `PreviewRuler` | `Widget` | 叶子 |
| `dropdown.hpp` | `Dropdown` | `Widget` | 弹层宿主 |
| `dropdown.hpp` | `detail::DropdownScrim` | `Widget` | 内部件 |
| `dropdown.hpp` | `detail::DropdownList` | `Widget` | 内部件 |
| `menu_bar.hpp` | `MenuBar` | `Widget` | 弹层宿主 |
| `menu_bar.hpp` | `detail::MenuScrim` | `Widget` | 内部件 |
| `menu_bar.hpp` | `detail::MenuPopup` | `Widget` | 内部件 |
| `list_view.hpp` | `ListView` | `Widget` | 虚拟滚动列表 |
| `table.hpp` | `Table` | `ListView` | 列表派生 |
| `tree_view.hpp` | `TreeView` | `Widget` | 虚拟滚动树 |
| `timeline_view.hpp` | `TimelineView` | `Widget` | 领域视图 |
| `scene_view.hpp` | `SceneView` | `Widget` | 领域视图 |
| `file_browser.hpp` | `FileBrowser` | `Widget` | 领域视图 |
| `debug_overlay.hpp` | `DebugOverlay` | `Panel` | 调试视图 |
| `ui_system.hpp` | `UISystem` | —（非控件，事件/命中/绘制驱动） | 系统层 |

---

## 3. 继承边（is-a）

```
Button        → Widget        Checkbox      → Widget
Radio         → Widget        Slider        → Widget
DragFloat     → Widget        ColorInput    → Widget
TextInput     → Widget        Progress      → Widget
Image         → Widget        ColorRect     → Widget
Splitter      → Widget        Tooltip       → Widget
Label         → Widget        Spacer        → Widget
CollapsibleHeader → Widget    PreviewRuler  → Widget
ScrollView    → Widget        Section       → Widget
直接继承 `Widget` 的其余控件：TreeView、TimelineView、SceneView、FileBrowser、Grid、Wrap、Hbox、Vbox、TabBar、TabView、Window、Scrim、DialogHost、ListView、MenuBar、Dropdown、UIRoot。

Heading       → Label         Paragraph     → Label
Panel         → Vbox          RadioGroup    → Vbox
List          → ScrollView    Table         → ListView
Dialog        → Panel         DebugOverlay  → Panel
ScrollPanel   → Panel

detail::DropdownScrim → Widget
detail::DropdownList  → Widget
detail::MenuScrim     → Widget
detail::MenuPopup     → Widget
```

非控件基类：`Widget → graphics::Drawable`（`src/graphics/drawable.hpp`，属外部依赖）。

---

## 4. include 边（`src/ui` 内部，编译期）

| 源 | 内部 include |
| --- | --- |
| `widget.hpp` | rect、align、uicontext、style |
| `uicontext.hpp` | theme |
| `style.hpp` | theme |
| `scroll_state.hpp` | scroll_mode |
| `grid.hpp` | widget、align |
| `wrap.hpp` | widget、align |
| `vbox.hpp` | widget |
| `hbox.hpp` | widget |
| `root.hpp` | widget |
| `spacer.hpp` | widget |
| `image.hpp` | widget |
| `color_rect.hpp` | widget |
| `scrim.hpp` | widget |
| `scene_view.hpp` | widget |
| `scroll_view.hpp` | widget、theme、scroll_mode |
| `scroll_view.cpp` | scroll_view、style |
| `panel.hpp` | theme、style、vbox |
| `scroll_panel.hpp` | panel、scroll_view、vbox |
| `list.hpp` | scroll_view、vbox |
| `list_view.hpp` | widget、scroll_state |
| `table.hpp` | theme、style、widget、list_view |
| `tree_view.hpp` | theme、style、widget、scroll_mode |
| `timeline_view.hpp` | theme、style、widget |
| `label.hpp` | theme、widget |
| `typography.hpp` | label、theme |
| `button.hpp` | theme、style、widget |
| `checkbox.hpp` | theme、style、widget |
| `radio.hpp` | theme、style、widget |
| `radio_group.hpp` | theme、vbox、radio |
| `slider.hpp` | theme、style、widget |
| `drag_float.hpp` | theme、style、widget |
| `color_input.hpp` | theme、style、widget |
| `text_input.hpp` | theme、style、widget |
| `progress.hpp` | theme、style、widget |
| `splitter.hpp` | theme、style、widget |
| `tooltip.hpp` | theme、style、widget |
| `preview_ruler.hpp` | theme、style、widget |
| `collapsible_header.hpp` | theme、widget |
| `section.hpp` | theme、collapsible_header、vbox、widget |
| `tab_bar.hpp` | theme、style、widget |
| `tab_view.hpp` | tab_bar、theme、widget |
| `window.hpp` | vbox、root、style、theme、widget |
| `dialog.hpp` | button、hbox、label、panel、root、scrim、theme、vbox |
| `dropdown.hpp` | theme、style、widget、root |
| `menu_bar.hpp` | root、theme、style、widget |
| `file_browser.hpp` | table、theme、widget |
| `debug_overlay.hpp` | button、checkbox、color_rect、hbox、label、panel、scroll_view、slider、spacer、splitter、text_input、tooltip、vbox |
| `ui_system.hpp` | root |
| `ui_system.cpp` | ui_system |
| `root.cpp` | root、scrim |
| `grid.cpp` / `hbox.cpp` / `vbox.cpp` / `wrap.cpp` | 各自头文件 |

`dialog.hpp` include 了 `scrim.hpp`，但在去注释后的代码中未出现 `Scrim` 类型使用（仅注释提到遮罩绘制复用）。

---

## 5. 组合边（has-a，持有或内部构造其它控件）

| 宿主 | 被持有控件 | 形式 |
| --- | --- | --- |
| `Widget` | `Widget` | `children_`：`std::vector<std::unique_ptr<Widget>>`（由所有容器复用） |
| `UIRoot` | `Widget` | `overlays_`、`modals_`、`modal_scrims_`、`trash_` |
| `UIRoot`（`root.cpp`） | `Scrim` | 模态入栈时 `std::make_unique<Scrim>()` |
| `Panel` | `Vbox` | 继承 |
| `ScrollPanel` | `ScrollView`、`Vbox` | 构造 `unique_ptr<ScrollView>` 与 `unique_ptr<Vbox>`，保存裸指针 `scroll_`、`content_` |
| `List` | `ScrollView`（继承）、`Vbox` | 构造 `unique_ptr<Vbox>` 作内容容器，保存 `Vbox* content_` |
| `RadioGroup` | `Vbox`（继承）、`Radio` | 构造若干 `unique_ptr<Radio>`，另存 `std::vector<Radio*> radios_` |
| `Section` | `CollapsibleHeader`、`Vbox` | `CollapsibleHeader header_`（值成员，不进入 `children_`）+ `unique_ptr<Vbox> content_` |
| `TabView` | `TabBar` | `children_[0]` 恒为 `TabBar`，经 `static_cast<TabBar*>` 访问 |
| `Window` | `Vbox` | `unique_ptr<Vbox> body_`，经 `adopt()` 挂入 |
| `Dialog` | `Panel`（继承）、`Label`、`Vbox`、`Hbox`、`Button` | 构造标题 `Label`、正文 `Vbox`、动作区 `Hbox`、多个 `Button`；保存 `Label* title_`、`Label* message_`、`Vbox* body_`、`Hbox* actions_`、`Button* primary_` |
| `DialogHost` | `Dialog`、`Widget` | `unique_ptr<Dialog>` 与全屏容器 |
| `DebugOverlay` | `Panel`（继承）、`Label` | 构造 `unique_ptr<Label> label_` |
| `FileBrowser` | `Table` | 构造 `unique_ptr<Table>`，保存 `Table* table_` |
| `Dropdown` | `detail::DropdownList`、`detail::DropdownScrim` | 打开时 `make_unique` 后 `UIRoot::add_overlay`，保存 `popup_`、`scrim_` |
| `MenuBar` | `detail::MenuPopup`（含子菜单链）、`detail::MenuScrim` | 同上，经 `UIRoot::add_overlay` 管理；`MenuPopup` 内部再持有 `MenuPopup` 子菜单 |
| `detail::DropdownList` | `Dropdown` | 持有 `Dropdown& owner_` 反向引用 |
| `detail::MenuPopup` | `MenuBar`、`MenuPopup` | 持有 `MenuBar& owner` 与子弹层指针 |
| `Table` | `ListView`（继承）、`ScrollState` | 使用 `ListView` 的 `protected ScrollState scroll_` |
| `ListView` | `ScrollState` | 成员 `scroll_` |

`CollapsibleHeader` 回调中引用 `Section` 语义，但源码中 `Section` 仅出现在注释（已剥离后无类型引用）。

---

## 6. 类型使用边（U，不持有实例）

| 使用方 | 被使用类型 | 位置 |
| --- | --- | --- |
| `DialogHost` | `UIRoot` | `find_root()` 中沿 parent 链 `dynamic_cast<UIRoot*>` |
| `Window` | `UIRoot` | `find_root_widget()` 中 `dynamic_cast<UIRoot*>`（另有两处注释提及） |
| `Dropdown` | `UIRoot` | `find_root()` 中 `dynamic_cast<UIRoot*>`，用于 `add_overlay` / `remove_overlay` |
| `MenuBar` | `UIRoot` | `find_root()` 中 `dynamic_cast<UIRoot*>`，用于 overlay 增删与视口宽度 |
| `Scrim` | `UIRoot` | 尺寸由 `UIRoot::layout_child` 特判为全屏（注释约定，无类型引用） |
| `DebugOverlay` | `Label`、`Button`、`Checkbox`、`Slider`、`TextInput`、`Tooltip`、`ScrollView`、`Panel`、`Vbox`、`Hbox`、`ColorRect`、`Spacer`、`Splitter` | 按控件类型名返回字符串（`dynamic_cast` 判别链），兜底返回 `"Widget"` |
| `typography.hpp` | `Label` | `apply_text_role(Label&, TextRole)` 参数，`Heading`/`Paragraph` 基类 |
| `Table` | `ListView` | 基类 + `protected` 成员访问 |
| `UISystem` | `UIRoot` | `unique_ptr<UIRoot> root_`、构造函数参数与 `root()` 返回值 |
| `UISystem` | `Widget` | 命中/事件/绘制的目标类型 |
| `Widget` | `UIContext`、`Theme`、`StyleRole` | `set_context`、`ui_theme`、`visual_style` |
| `TreeView` | `ScrollMode` | 滚动模式枚举 |
| `ScrollView` | `ScrollMode` | 滚动模式枚举 |
| `style.hpp` / `theme.hpp` / `slider.hpp` | `StyleRole::Tooltip`、`StyleRole::Row` 等 | 样式角色枚举值，**不是** `Tooltip` 控件类型 |

未发现控件间引用的候选项（经去注释校验）：`collapsible_header.hpp` 中 `Section`/`Window`、`scroll_state.hpp` 中 `Table`/`ScrollView`/`TextInput`、`radio.hpp` 中 `Checkbox`、`list_view.hpp` 中 `Table`/`TreeView`、`color_input.hpp` 中 `TextInput`、`scene_view.hpp` 中 `Image`（实为 `scene::NodeType::Image`）、`widget.hpp` 中 `FileBrowser`/`Panel`/`ScrollView`/`Table`/`UISystem`，均只出现在注释或枚举名中。

---

## 7. 内部子控件（detail）依赖

| 内部类 | 依赖 | 方向 |
| --- | --- | --- |
| `detail::DropdownScrim` | `Widget`（继承）、通过 `std::function` 回调 dismiss | 出 → 宿主 |
| `detail::DropdownList` | `Widget`（继承）、`Dropdown& owner_` | 双向：宿主持有列表，列表回调宿主 |
| `detail::MenuScrim` | `Widget`（继承）、dismiss/hover 回调 | 出 → 宿主 |
| `detail::MenuPopup` | `Widget`（继承）、`MenuBar& owner`、子 `MenuPopup`、`MenuItem` 数据 | 双向 + 自嵌套 |

`MenuBar` 声明 `friend class detail::MenuPopup`（`menu_bar.hpp:413`）。

---

## 8. 反向入度（被依赖次数，合并 is-a / H / U）

| 被依赖类型 | 次数 | 依赖方 |
| --- | --- | --- |
| `Widget` | 全部 | 所有控件（继承或持有） |
| `Vbox` | 7 | panel(is-a)、scroll_panel、list、section、radio_group(is-a)、window、dialog |
| `UIRoot` | 5 | dialog、window、dropdown、menu_bar、ui_system |
| `Label` | 4 | typography(is-a×2)、dialog、debug_overlay |
| `ScrollView` | 3 | list(is-a)、scroll_panel、debug_overlay |
| `Hbox` | 2 | dialog、debug_overlay |
| `Button` | 2 | dialog、debug_overlay |
| `ScrollState` | 1 | list_view（成员 `scroll_`）；table 经 `ListView` 继承的 `protected` 成员间接使用，未出现类型名 |
| `ScrollMode` | 2 | scroll_view、tree_view |
| `ListView` | 1 | table(is-a) |
| `Panel` | 4（3 个依赖方） | dialog(is-a)、debug_overlay(is-a + U)、scroll_panel(is-a) |
| `Table` | 1 | file_browser |
| `Radio` | 1 | radio_group |
| `CollapsibleHeader` | 1 | section |
| `TabBar` | 1 | tab_view |
| `Dialog` | 1 | dialog 宿主 |
| `Scrim` | 1 | root.cpp（UIRoot 模态栈） |
| `Tooltip`、`Checkbox`、`Slider`、`TextInput`、`ColorRect`、`Spacer`、`Splitter` | 各 1 | debug_overlay（仅类型识别） |

零内部依赖（未引用其它控件类型，只依赖 Widget/theme/style/graphics）：`Button`、`Checkbox`、`Radio`、`Slider`、`DragFloat`、`ColorInput`、`TextInput`、`Progress`、`Image`、`ColorRect`、`Splitter`、`Spacer`、`Label`、`CollapsibleHeader`、`PreviewRuler`、`TabBar`、`Tooltip`、`Scrim`、`TimelineView`、`SceneView`、`TreeView`、`Grid`、`Hbox`、`Vbox`、`Wrap`。`UIRoot` 自身不引用具体控件类型，仅其实现文件 `root.cpp` 引用 `Scrim`。

---

## 9. 双向 / 成对关系（仅列事实）

| 关系 | 组成 |
| --- | --- |
| 继承 + 成员叠加 | `Table` ⊃ `ListView` ⊃ `ScrollState`；`List` ⊃ `ScrollView` |
| 容器叠加 | `ScrollPanel` ⊃ `Panel` ⊃ `Vbox`；`Dialog` ⊃ `Panel` ⊃ `Vbox` |
| 面板 + 内容分离 | `Section`（`CollapsibleHeader` + `Vbox`）；`TabView`（`TabBar` + 页面） |
| overlay 弹层对 | `Dropdown` ↔ `detail::DropdownList` / `detail::DropdownScrim`；`MenuBar` ↔ `detail::MenuPopup` / `detail::MenuScrim` |
| 模态对 | `UIRoot` ↔ `Scrim`；`DialogHost` ↔ `Dialog` |
| 反向引用 | `detail::DropdownList → Dropdown`、`detail::MenuPopup → MenuBar`（`friend`） |
| 只读识别 | `DebugOverlay` → 13 个具体控件类型（`dynamic_cast` 判别） |

---

## 10. 层级视图

```
[0] rect / align / theme / style / uicontext / scroll_mode / scroll_state
              │
[1]        Widget ─────────────────────────────────────────────┐
              │                                                │
[2]  Vbox  Hbox  Grid  Wrap  Spacer  ScrollView  Scrim  ColorRect  Image
       │      │                                    │
[3]  Panel   （叶子控件）      List               ScrollPanel
       │                        │                    │
[4]  Dialog  DebugOverlay     （Vbox 内容）      ScrollView + Vbox
       │
[5]  DialogHost → UIRoot(overlay/模态) ← Scrim

[3'] Section(Vbox + CollapsibleHeader)、TabView(TabBar)、Window(Vbox body)、
    RadioGroup(Vbox + Radio)、FileBrowser(Table → ListView → ScrollState)

[4'] Dropdown / MenuBar → UIRoot overlay 上的 detail 弹层与遮罩

[并列] Label → Heading / Paragraph；ListView → Table；ScrollView → List
```

---

## 11. 外部依赖边界（控件引用的 `src/ui` 之外头文件）

| 外部头 | 引用方 |
| --- | --- |
| `graphics/drawable.hpp` | `widget.hpp` |
| `graphics/box.hpp` | 29 个文件（含 widget.hpp 与多数控件，绘制盒模型） |
| `graphics/rectangle.hpp` | 16 个文件：collapsible_header、color_rect、dialog、dropdown、menu_bar、preview_ruler、scene_view、scrim、scroll_view.cpp、splitter、tab_view、table、text_input、timeline_view、tree_view、window |
| `graphics/line.hpp` | 8 个文件：collapsible_header、menu_bar、section、timeline_view、tree_view、vbox、widget、window |
| `graphics/triangle.hpp` | dropdown、menu_bar、radio |
| `graphics/font_atlas.hpp` | button、checkbox、collapsible_header、dropdown、label、menu_bar、radio、tab_bar、table、text_input、tooltip、tree_view |
| `graphics/text_debug.hpp` | button、label、window |
| `graphics/utf8.hpp` | label、text_input |
| `graphics/sprite.hpp` | scene_view |
| `graphics/frame_stats.hpp` | debug_overlay |
| `events/event.hpp` | color_input、drag_float、list_view、timeline_view、ui_system |
| `events/key_code.hpp` | widget、uicontext |
| `anim/scene.hpp` | scene_view |
| `anim/timeline.hpp` | timeline_view |
| `backend/window.hpp` | ui_system.cpp |
| `<glm/glm.hpp>`（尖括号包含） | rect、theme |
