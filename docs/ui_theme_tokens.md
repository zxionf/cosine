# 主题令牌对照表（UI-11 / UI-39）

> 范围：`src/ui/theme.hpp` 的 `Theme` 字段与其消费方。
> 规则：**每个样式令牌必须映射到 ≥1 个 `StyleRole`，或明确标注为「度量 / 策略 / 布局」。**
> 只被赋值、无人读取的令牌视为死令牌，必须在同一任务内删除。

## 1. 三层结构

| 层 | 令牌 | 消费方式 |
|---|---|---|
| **样式** | 颜色 + 圆角档位 | 经 `StyleRole` 解析（`Widget::visual_style`）或直接读令牌 |
| **度量** | 尺寸 / 字号 / 间距 | 控件直接读（无角色语义） |
| **策略** | 滚动 / 焦点 / 密度 / 过渡 | 控件或系统按开关分支 |

## 2. 样式令牌 → 角色

`StyleRole` 有 12 个：`Surface` / `Control` / `Input` / `Track` / `Accent` / `Row` / `MenuItem` /
`Tooltip` / `Chrome` / `RowSelected` / `Knob` / `Thumb`，解析实现在 `src/ui/style.hpp` 的
`Theme::style(role, state)`。
`ControlState` 有 5 个：`Normal` / `Hovered` / `Pressed` / `Disabled` / `Checked`（UI-40 新增）。

> **同一帧不变量**：一个控件在同一帧内，一个角色只能用一个状态。
> 过渡槽按角色索引（不是按状态），同帧内对同一角色先后请求两种状态会互相覆盖，
> 两处都拿到中间色。需要两种外观时用两个角色（UI-46 的 `Track` + `Thumb` 即由此而来）。

| 令牌 | 角色 | 语义 | 备注 |
|---|---|---|---|
| `panel_bg` | `Surface` | 面板 / 卡片底 | — |
| `panel_border` | `Surface` | 面板描边（`Hovered` → accent） | 表格/弹层等仍借用它作描边 |
| `border` | 全角色 | 描边宽度（度量型样式） | 控件普遍读它画 1px 边框 |
| `radius*` | 全角色 | 圆角档位（`radius`/`_small`/`_medium`/`_panel`/`_pill`） | 控件按用途挑档位 |
| `button_bg` / `button_hover` / `button_pressed` / `button_disabled` | `Control` | 按钮类四态（`Checked` → accent） | — |
| `text_input_bg` / `text_input_border` / `text_input_border_hover` | `Input` | 表单框：三态 + `Checked`（编辑/展开中 → accent 边框） | Checkbox/Radio 框体也走它 |
| `slider_track` / `track_hover` | `Track` | 槽（滑轨 / 进度底 / 滚动条轨道）：常态 / hover（`Pressed` → accent） | **不含滚动条滑块**（UI-46 拆出 `Thumb`） |
| `thumb` / `thumb_hover` | `Thumb` | 滚动条滑块：常态 / hover（`Pressed` → accent） | 与 `Track` 槽对比度 ≥ 2.2:1（dark 2.31 / light 2.42） |
| `slider_handle` | `Knob` | 可拖动手柄（`Hovered`/`Checked` 在主题里向 accent 混） | Slider |
| `accent` | `Accent` / `RowSelected` / `Chrome:Checked` / `Track:Pressed` | 强调色 | 仍有 18 处直读（焦点环/播放头/选框等 accent 语义本身） |
| `accent_on` | `Accent` | accent 填充上的文字色（dark 5.50:1 / light 4.95:1） | 取代原硬编码 `0x0F1419` |
| `elevated` | 窗口底 | 浮起层底色 | `Window` 直接读（窗口整体不是角色） |
| `elevated_border` / `button_hover` | `Chrome` | 窗口外框 / 标题栏底色（`Checked` → accent 外框） | `Window` |
| `tooltip_bg` | `Tooltip` | 浮层底 | — |
| `splitter` | （应用级） | 分割条本体色 | `Splitter` |

> 读表方式：优先走 `visual_style(role, state)`；只要控件已有 `BoxStyle`，文字色就用 `style.text`
> （它已包含 Disabled 切换），不要再手写 `enabled_ ? t.text : t.text_disabled`。

## 3. 文字令牌

| 令牌 | 用途 |
|---|---|
| `text` | 正文色；`style()` 里按 `ControlState::Disabled` 自动切换成 `text_disabled` |
| `text_disabled` | 禁用态文字（也被 `Accent` 的禁用填充复用） |
| `text_muted` | 次要文字（`Image` 占位、`TreeView`/`MenuBar` 箭头、`Window` 图标） |
| `accent_text` | 「底色上的蓝字」：`DragFloat`、`TimelineView`、`Accent` 的边框 |

## 4. 度量令牌

### 字号档位（UI-41）

`BoxStyle` 除了 `text_size`（像素值）还带 `text_size_role`（`TextSize::{Body, Small, Heading}`）：
角色可以声明「这类控件用大字 / 小字」，控件读 `style.text_size` 即可，不再直读
`small_text_size` / `heading_text_size`。

```cpp
const BoxStyle tip = theme.style(StyleRole::Tooltip, ControlState::Normal);
tip.text_size_role == TextSize::Small;   // 角色声明
tip.text_size == theme.small_text_size; // 换算后的像素值
theme.text_size_of(TextSize::Small);    // 控件需要单点小字号时用这个
```

| 令牌 | 值（dark） | 主要消费方 |
|---|---|---|
| `text_size` | 16 | 几乎所有控件（99 处） |
| `small_text_size` | 12 | 经 `TextSize::Small` 取：Tooltip / PreviewRuler / TimelineView / Image 占位 / MenuItem 快捷键 |
| `heading_text_size` | 22 | 经 `TextSize::Heading`：`Editor` 面板标题 |
| `control_height` | 30 | 按钮 / 输入 / 下拉 / 滑块 / TabBar / 折叠头 |
| `row_height` | 26 | 列表行 / 菜单行 / 时间轴行 |
| `checkbox_size` | 18 | `Checkbox` / `Radio` |
| `slider_height` / `slider_handle_radius` | 6 / 7 | `Slider` / `Progress` |
| `scrollbar_width` / `scrollbar_min_thumb` | 8 / 20 | `ScrollView` / `ListView` / `Table` / `TreeView` |
| `padding` | 8 | 面板 / 按钮 / 输入 / 密集（59 处） |
| `spacing` | 6 | 容器间距 / 控件内元素间距（50 处） |
| `inset_shadow_size` | 6 | `Panel` 内阴影 |
| `resize_margin` | 4 | `Window` resize 命中带（`UISystem`） |

## 5. 布局度量（UI-35 引入，非样式）

这些字段由布局代码消费，不进 `StyleRole`；新增样式角色时不要读它们。

| 令牌 | 消费方 |
|---|---|
| `dialog_margin` / `dialog_min_width` / `dialog_min_height` | `DialogHost` |
| `window_min_width` / `window_min_extra_height` / `window_keep_visible` | `Window` |
| `splitter_handle` | `Splitter` |
| `section_inset` | `Section` |

## 6. 策略令牌

| 令牌 | 取值 | 消费方 |
|---|---|---|
| `scroll_mode` | Smooth / Fast / Line | `ScrollView`（未显式覆盖时）、`ScrollState`、`Table` |
| `focus_style` | Outline / Border | `Widget::append_focus_ring` |
| `focus_policy` | Keyboard / Always / Never | `Widget::focus_visible`、`Dropdown`、`Editor` |
| `focus_thickness` / `focus_offset` | 2 / 2 | `Widget::append_focus_ring` |
| `transition_ms` | 120 | `Widget::transition_duration` |
| `pixel_snap` | true | `Label`、`Editor` |
| `density` | Compact / Normal / Spacious | `Theme::with_density`（**唯一密度机制**，UI-43） |
| `name` | "dark" / "light" | **显示用途**（About 页）；不参与样式解析 |

## 7. 死令牌已删除（UI-39）

已删除并在 `test_ui_style` 里用对比度/角色断言钉住语义：

| 令牌 | 原状 | 处置 |
|---|---|---|
| `Theme::focus_ring` | 只在 `theme.cpp` 赋值，读取 0 | 删（焦点环实际用 `accent`） |
| `BoxStyle::focus_ring` | 被 `lerp_style` 混合，无人绘制 | 删（`append_focus_ring` 直接读 `accent`） |
| `Theme::slider_fill` | 读取 0 | 删（填充走 `Accent`） |
| `Theme::scrollbar_track` | 读取 0 | 删（轨道走 `Track`） |
| `Theme::radius_large` | 读取 0 | 删（无人使用的档位） |
| `Theme::checkbox_fill` | UI-38 后读取 0（改走 `Accent`） | 删（与 `accent` 同值 `0x3D8EE8`） |
| `Theme::name` | 仅供显示（About 页） | **保留**，不参与样式解析 |

### 对比度（UI-39 新增令牌）

`accent_on` = accent 填充上的文字色，由 `test_ui_style::test_accent_on_contrast` 断言：

| 主题 | accent | accent_on | 对比度 |
|---|---|---|---|
| dark | `#3D8EE8` | `#0F1419` | **5.50:1** ✓ |
| light | `#1F6FD0` | `#FFFFFF` | **4.95:1** ✓ |

修复前 `Accent` 角色把文字色硬编码为 `#0F1419`，浅色主题只有 **3.74:1**（不达 WCAG AA 4.5:1）。

## 8. UI-40 收敛结果

| 原现象 | 位置 | 现在 |
|---|---|---|
| 标题栏底色借 `button_hover`、边框手写 `emphasized ? accent : elevated_border` | `window.hpp` | **`Chrome` 角色**（`Checked` = 活跃/聚焦） |
| 滚动条滑块直读 `scrollbar_thumb`，轨道走 `Track` | `scroll_view.cpp`、`list_view.hpp` | **槽 `Track:Normal` / 滑块 `Thumb`**（UI-46：借用 `Track:Hovered` 会被同帧的槽请求覆盖，滑块看不见） |
| 勾选框体直读 `checkbox_bg/border` + hover 三元式 | `checkbox.hpp`、`radio.hpp` | **`Input` 角色**（与 TextInput 同口径），两令牌已删 |
| 手柄 `mix_color(handle, accent, 0.15/0.35)` | `slider.hpp` | **`Knob` 角色**（`Hovered`/`Checked` 在主题里做混合），控件零手写 |
| Panel hover 边框三元式 | `panel.hpp` | **`Surface:Hovered`** |
| 行选中叠 `accent` 半透明 | `table.hpp`、`tree_view.hpp` | **`RowSelected` 角色**（alpha 0.28 保持不变） |
| 输入框/下拉的 `is_focused() ? accent : border` | `text_input.hpp`、`dropdown.hpp` | **`Input:Checked`** |
| 已有 `BoxStyle` 却仍直读文字色 | `drag_float` / `color_input` / `text_input` / `table` / `tree_view` / `window` / `collapsible_header` | 统一取 `style.text` |

### 量化

| 指标 | UI-40 前 | 现在 |
|---|---|---|
| 颜色类令牌直读（21 个令牌合计） | 129 | **76（−41%）** |
| 手写状态三元式（`is_hovered() ? ControlState`） | 6 | **0** |
| 控件内手写 `mix_color`（角色色） | 3 | **0** |
| 样式角色 | 8 | **11**（+Chrome / RowSelected / Knob） |
| `ControlState` | 4 | **5**（+Checked） |

剩余 76 处的构成：`accent` 18（焦点环、播放头、时间轴选框、Tab 下划线、拖动环——都是 accent 语义本身）、
`text*` 12（无 `BoxStyle` 上下文的散点）、其余为 `panel_border` / `splitter` / 度量类令牌。
强行再抽象会变成“角色里只剩一个令牌”的反向复杂度。

## 8b. UI-43 密度与状态语义

| 项 | UI-43 前 | 现在 |
|---|---|---|
| 密度机制 | 两套（`Theme::with_density` + `Editor::kDensity` 表，数值不一致） | **只有 `Theme::with_density`** |
| `Row:Disabled` | 无定义（落到 default） | 填充 = `button_disabled`、文字 = `text_disabled` |
| `MenuItem:Disabled` | 填充透明、文字直读令牌 | 填充透明、文字/字号走 `MenuItem` 角色 |
| 选中行 | 控件内手叠 `accent*0.28` | `RowSelected` 角色（alpha 仍 0.28） |
| 验收 | — | `test_ui_style::test_state_coverage` 逐主题断言各状态互不相同 |

## 9. 复查命令

```bash
# 颜色令牌直读（UI-40 后应 <= 80）
for t in accent text text_disabled panel_bg panel_border; do grep -rhoE "\bt\.$t\b" src/ui | wc -l; done
# 死令牌（应为 0）
grep -rnw 'slider_fill\|scrollbar_track\|checkbox_fill\|checkbox_bg\|checkbox_border\|scrollbar_thumb\|radius_large' src | grep -v theme
# 手写状态三元式（应为 0）
grep -rn 'is_hovered() ? ControlState' src/ui | wc -l
# 快照（深/浅两套）
ctest -R ui_style_snapshot --output-on-failure
```
