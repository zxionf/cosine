# src/ui 重新设计（UI-R）

> 版本：v1（2026-11-27 15:17 CST）
> 基线：main@167a3f83（工作区另有 `src/ui/widget.hpp` 未提交改动：`x/y/width/height` 去 `virtual`）
> 范围：**只覆盖 `src/ui/`**（66 文件 / 13,215 行 / 43 个控件类）。
> 不改 `docs/` 下旧文档；`docs/ui_architecture.md` 保留为"现状描述"，与本文件冲突处以本文件为准（冲突点见 §12）。

---

## 0. 一句话

把 `src/ui` 从「一个 630 行的 `Widget` 基类 + 42 个子类各自抄样板」改成
**「4 类薄节点（Node / LayoutNode / LeafNode / ContainerNode）+ 4 个按值内嵌的部件（LayoutParams / BoxMetrics / Interaction / VisualState）+ 1 个派生语义投影（Semantics）」**。

首要不变量：**一个事实只有一个所有者**；次要目标：**新控件 ≤ 60 行、不新增基类虚函数**。

---

## 1. 目标（可量化）与不追求

| 指标 | 现状（实测） | 目标 |
|---|---|---|
| 单文件最大行数 | `widget.hpp` 630 | **≤ 250**（每个头都可独立读懂） |
| 基类虚函数总数 | 50（含 16 个从未被覆写的虚函数） | **R1 后 34**；可选 R7 降到 ≤ 30 |
| `arrange()` 纯样板 | 17 / 35 个控件（已清零，见 R1 记录） | **0** |
| 状态字段副本 | `enabled_` 8、`selected_` 6+1、`pressed_` 3、`dragging_` 5 | **各 1（`Interaction`）** |
| 测量缓存 | 3 套（基类 / `Label` / `TreeView`） | **1 套（key 可扩展）** |
| 过渡缓存内存 | 每个交互过的控件 **1,688 B + 1 次堆分配** | **≤ 128 B、0 堆分配、可共享** |
| 绕过 `set_geometry` 的几何写 | 11 处（`widget.hpp` 外） | **0** |
| 树遍历方式 | 2 种（虚 `children()` / 成员 `children_`），4 个遍历点 | **1 种（`for_each_child`）** |
| 新增"可点/可禁用/带焦点环"控件 | 90~130 行 + 4 个钩子 | **≤ 60 行 + 0 钩子** |

**不追求**：

- 不追求总行数下降（算法本身正确，13,215 行里大部分是业务绘制）。
- 不追求"一个类一个文件"。
- 不引入新范式：不做 Flutter 三段式（Widget/Element/RenderObject）、不做约束求解、不做立即模式。
- 不改变布局语义（`measure` 向下传约束、`arrange` 写几何、`measure_child` 统一扣 margin / pct / fill / aspect / clamp）——这套已被 74 个测试与快照锁定。

---

## 2. 设计原则（4 条）

1. **语义分层**：几何、树、布局、输入、样式、语义各自成为一层；层内单向依赖。
2. **组合优于继承**：可变点在**部件**里（按值内嵌，零堆分配），不在基类虚函数里。
3. **单一真相**：每个事实只有一个写入口；派生物只读（`Semantics` 是投影，不是存储）。
4. **契约可测**：每条设计约束都能用一行 `grep` 或一个测试断言守住，否则不算契约。

---

## 3. 目标目录与依赖规则

### 3.1 已落地结构（UI-53 as-built）

```
ui/
  geom/        align.hpp rect.hpp layout_metrics.hpp layout_budget.hpp        纯数据 + 纯函数
  layout/      layout_params.hpp measure_cache.hpp layout_utils.hpp
               scroll_mode.hpp scroll_state.hpp
  style/       style.hpp style_types.hpp theme.{hpp,cpp} widget_style.hpp
               visual_state.hpp box_metrics.hpp
  input/       interaction.hpp
  node/        widget.{hpp,cpp} leaf_node.hpp
  semantics/   semantics.hpp
  system/      root.{hpp,cpp} ui_system.{hpp,cpp} uicontext.hpp
  core/ containers/ composites/ views/      （未动：控件实现本体）
```

旧路径在 R7（UI-53）时曾是**转发头**（一行 `#include` + 标记），R8（UI-54）已彻底删除：
259 处 include 全部改到新路径，`src/ui` 根目录不再有任何头/源文件。
`test_ui_architecture` 的 D5 断言现在锁住“根目录无头文件 + 无转发头”。
259 处 include（已登记为 R7，见 `docs/todo-ui.md`）。

### 3.2 目标结构（原设计，含尚未拆出的 widgets/ 与 tree/）

```
ui/
  geom/        Rect Edges Align layout_metrics layout_budget          纯数据 + 纯函数
  layout/      LayoutParams MeasureCache LinearLayout Grid Wrap ScrollMetrics
  tree/        ChildList(for_each_child/adopt/paint_order) DirtyGraph LayoutEpoch
  input/       Interaction PointerTarget KeyTarget FocusTarget
  style/       StyleRole ControlState TextSize Theme StyleResolver BoxPainter TextPainter
  node/        Node LayoutNode LeafNode ContainerNode
  semantics/   Semantics
  widgets/     Button Label Slider Checkbox Radio Progress …          只装配
  containers/  Vbox ScrollView Splitter Panel
  composites/  Window MenuBar TabView Table Dialog …
  system/      UIRoot UISystem（dispatch / focus / modal / dpi）
```

**层内依赖规则（已由 `test_ui_architecture` 断言）**：

| # | 规则 | 状态 |
|---|---|---|
| D1 | `geom/` 与 `layout/measure_cache.hpp` 不得 include 控件树 | ✅ 断言守住 |
| D2 | `core/` `containers/` 不得 include `composites/` `views/` | ✅ 断言守住 |
| D3 | `style/` 不得 include 具体控件 | ✅ 断言守住 |
| D4 | `semantics/` 只依赖标准库 | ✅（semantics.hpp 只 include \<cstdint\>/\<string_view\>） |
| D5 | `src/ui` 根目录不得有头/源文件，不得有转发头（旧路径彻底退场） | ✅ 断言守住（UI-54 后） |

> 断言实现：`test/test_ui_architecture.cpp` 直接读源文件做文本断言（不引入脚本，符合 AGENTS 第 9 条）。

---

## 4. 类型模型

### 4.1 `Node`（身份 + 几何 + 树 + 层叠）— 目标 ≤ 180 行

```cpp
class Node : public graphics::Drawable {
public:
    // 身份与几何（只读）
    Rect local() const;  Rect global() const;
    void set_geometry(Rect slot);        // 唯一写入口（替代 11 处直写 global_）

    // 树（唯一入口）
    void adopt(std::unique_ptr<Node> child);
    template <class F> void for_each_child(F&& f) const;   // 含 overlays
    Node* parent() const;
    const std::shared_ptr<const bool>& alive_token() const; // UISystem 悬垂检测

    // 层叠与可见性（语义，不含机制）
    Display display() const;  Overflow overflow() const;  Layer layer() const;
    bool clips_children() const;          // 非虚：由 overflow() 派生（现状是虚，且 0 覆写）

    // 语义投影
    virtual Semantics semantics() const;  // 默认 {role=Custom}
protected:
    Geometry geometry_;                    // private 语义：只经 set_geometry 写
    ChildList children_;                   // private：parent/token/adopt/paint_order
};
```

要点：

- **`clips_children()` 从虚函数降为普通函数**（它由 `overflow()` 派生，0 个覆写）。
- **`geometry_` 私有**：读用 `local()/global()`（255 处读点不动），写只留 `set_geometry()`（11 处写点要改）。
- **`children_` 私有**：读用 `for_each_child()` / `child_count()` / `child_at(i)`（76 处引用要迁，其中 `splitter` 25、`tab_view` 15、`grid.cpp` 11）。
- `Semantics` 旁挂（§4.5），不增加存储。

### 4.2 `LayoutNode`（布局机制 + 3 个部件）— 目标 ≤ 220 行

```cpp
class LayoutNode : public Node {
public:
    glm::vec2 measure(glm::vec2 constraints) override = 0;   // 保留
    void      arrange(Rect slot) override = 0;               // 保留

    // 字段名用 layout_ 而非 layout（UI-48 实测）：Widget 已有虚函数 layout()，同名会冲突。
    LayoutParams  layout_;   // 原 15 个字段：preferred/min/max/pct/fill/aspect/flex/align/absolute
    BoxMetrics    box;       // 原 3 个字段：padding / padding_override / margin
    Interaction   input;     // 状态唯一所有者（见 §4.3）
    VisualState   visual;    // 样式过渡（见 §4.4，内嵌、无堆）

protected:
    // 测量缓存：唯一一套（见 K6）——ui/measure_cache.hpp 的 MeasureCache
    MeasureCache cache_;
};
```

`LayoutParams` / `BoxMetrics` 只是**数据搬家**：现有访问器 `min_size()/set_min_size()/fill_x()/padding()/…` 保留签名，内部转发到部件。这样 43 个控件里 0 处访问点需要改。

### 4.3 `Interaction`（状态唯一所有者）— 目标 ≤ 90 行

```cpp
struct Interaction {
    bool enabled  = true;   // 收敛 8 份 enabled_
    bool hovered  = false;
    bool pressed  = false;
    bool dragging = false;  // 收敛 5 份 dragging_
    bool selected = false;  // 收敛 6 份 selected_（列表类保留自己的索引，语义投影到这里）
    bool focused  = false;

    ControlState state(bool checked = false) const;  // 取代 style.hpp:50 的自由函数
};
```

- `UISystem` 继续直接写 `hovered/focused`（职责不变，只是写入点从控件字段换成 `input.`）。
- `control_state(enabled, hovered, pressed, checked)` 自由函数**删除**，改为 `input.state(checked)`（13 处 `pressed_` 赋值点同时收敛）。
- 列表类（`ListView` 的 `int selected_`、`TabBar`/`Dropdown`/`RadioGroup` 的 `int selected_`、`TimelineView` 的 `selected_track_`）**保留索引**，通过 `Interaction::selected` 暴露"是否有选中"给语义层——索引是它们的数据，语义是统一视图。

### 4.4 `VisualState`（样式过渡：从 1,688 B 降到 104 B、去堆分配）

旧实现：`StyleTransition` 内含 12 个 `StyleSlot`，每槽 `from/to` 各一个 `BoxStyle`（64 B）→ `sizeof = 1,688`，还要 `unique_ptr` 惰性堆分配。

新实现（UI-50 已落地）：**槽里只存「状态对 + 进度」**，颜色在渲染时按 `(role, state)` 从当前 Theme 现算：

```cpp
struct StyleSlot {                 // 实测 8 B
    ControlState from = ControlState::Normal;  // 1 B（enum 底层类型固定为 uint8_t）
    ControlState to   = ControlState::Normal;  // 1 B
    bool started = false;                      // 1 B（首次使用：吸附，不做淡入）
    bool reversing = false;                    // 1 B（反向过渡：进度往回走）
    float progress = 1.0f;                     // 4 B
};
struct VisualState {               // 实测 104 B（12 × 8 + revision 8）
    mutable StyleTransition transition;        // 内嵌，不再 unique_ptr
};
```

与初版设计的差异（已实施后校正）：

| 初版 | 实际 | 原因 |
|---|---|---|
| Theme 里建 12×5 = 60 项样式索引表 | **不建表**，按需 `theme.style(role, state)` 现算 | 索引表要与 Theme 字段保持同步（`with_*` 每次改字段就得重建），而现算只有一次 switch；避免了“表是第二份真相”的风险 |
| `started` 位图 + 索引 | `started`/`reversing` 两个 bool 存在槽里 | 保留两个已确立的语义：首帧吸附、中途反向不跳变 |
| 中途反向从“当前插值色”再出发 | 反向 = 进度回退（`reversing`）；三状态快速交替时 from 取上一个目标状态 | 存中间色需要 48 B/槽；反向回退能精确覆盖 hover→press→hover 这类常见路径 |

内存对比（实测）：

| | 之前 | 之后 |
|---|---|---|
| `StyleSlot` | 140 B | **8 B** |
| `VisualState` | 1,688 B + 1 次 `new` | **104 B，0 次 `new`** |
| `sizeof(Widget)` | 288 B（+ 惰性 1,688） | **384 B（全含）** |
| 交互过的控件总量 | ≈ 1,970 B | **384 B（−81%）** |
| 从未动画的控件 | 288 B | 384 B（+96 B） |

取舍：用“所有控件 +96 B”换“交互控件 −1,586 B 且零堆”。后者是热点（每次 hover 过的控件），前者是一次性常数。

保持的语义：`(role, state)` 分槽、首帧吸附、中途反向不跳变、revision 变更逐槽重建、**一帧一角色一状态**（UI-46 教训已在 `widget_style.hpp` 注释与 `test_ui_visual_state` 里固化）。

### 4.5 `Semantics`（派生投影，不存第二份真值）

```cpp
enum class WidgetRole { Panel, Button, CheckBox, Radio, Slider, TextField, Label, Image,
                        List, Table, Tree, Menu, MenuItem, Dialog, Tooltip, Splitter,
                        Progress, ScrollView, Custom };

struct Semantics {
    WidgetRole role = WidgetRole::Custom;
    std::string_view label;      // 无障碍名 / 调试面板显示名
    std::string value;           // Slider/Progress 的当前值文本（派生）
    bool enabled = true;         // ← 从 input 派生
    bool focused = false;        // ← 派生
    bool selected = false;       // ← 派生
    bool checkable = false;
    bool expanded = false;
};
```

- `semantics()` **必须是 `const` 派生**：返回值里除 `role/label`（控件常量）外全部从 `input` 读取。
- 唯一价值：让通用代码（调试面板、Tab 顺序、测试断言、未来 a11y）**不必 `dynamic_cast`**。
- 不做平台 a11y 接入（无接入方时是纯负担）。

### 4.6 四类节点

| 类型 | 谁 | 必须实现 | 现在要实现 |
|---|---|---|---|
| LeafNode | Button Label Checkbox Radio Slider Progress Image ColorRect PreviewRuler Tooltip Spacer CollapsibleHeader DragFloat ColorInput TextInput TabBar Scrim SceneView TimelineView（19） | **只 `append_to`** | `measure` + `arrange` + `append_to`（其中 17 个 arrange 是 `set_geometry(slot);`，已由 LeafNode 吸收） |
| ContainerNode | LinearLayout(Hbox/Vbox) Grid Wrap Panel ScrollView ScrollPanel Splitter Section TabView Window DialogHost Dropdown MenuBar FileBrowser RadioGroup（17） | `measure` + 子项摆放 | `measure` + `arrange` + 全部手写子项遍历 |
| `ListView`（DataView 角色，已存在） | ListView / Table / TreeView | `item_count` + `draw_row` | 已是（UI-36 的成果，保留） |
| 浮层角色 | Scrim Tooltip MenuPopup MenuScrim DropdownList | 同 LeafNode + `layer()` | 已有 |

`LeafNode` 的默认实现（已吃掉 17 个样板 + 5 个 `clamp_size(preferred_size())`）：

```cpp
class LeafNode : public LayoutNode {
public:
    void arrange(Rect slot) override { set_geometry(slot); }
    glm::vec2 measure(glm::vec2) override { return clamp_size(layout.preferred); }
};
```

### 4.7 两个绘制部件（消除 25 处 `visual_style` + 13 处焦点环）

```cpp
struct BoxPainter {   // fill + border + focus ring + 圆角档位
    void paint(graphics::DrawList&, Rect bounds, const BoxStyle&, bool focused) const;
};
struct TextPainter {  // 字号档位 + 基线 + debug 叠加
    glm::vec2 measure(std::string_view, const BoxStyle&, graphics::FontAtlas*) const;
    void paint(graphics::DrawList&, Rect, std::string_view, const BoxStyle&, DebugFlags) const;
};
```

- `BoxPainter` 按**值**内嵌（无状态，8~16 B）。
- `TextPainter` 同理（如需缓存由 `FontAtlas` 承担，不引入新缓存）。
- 收益：新控件的"边框 + 焦点环 + 文字居中"从 15 行降到 2 行。

---

## 5. 对象布局与预算

| 对象 | 现在 | 目标 | 说明 |
|---|---|---|---|
| `sizeof(Widget)` | 280 B | ≤ 300 B | 部件内嵌后**不显著增大**；字段只是从散落改成聚合 |
| `StyleSlot` / `VisualState` | 140 B / 1,688 B + `new` | **8 B / 104 B，0 次 `new`** |
| 交互过控件的总内存 | ≈ 1,970 B | **≈ 384 B（−81%）** |
| 每控件堆分配 | 1（交互后） | **0** |

**硬规则**：任何部件都不得是 `std::unique_ptr<Part>`；不得为"组合"引入逐部件的动态分配。

---

## 6. 六条契约（K1–K6）

| # | 契约 | 断言方式 |
|---|---|---|
| **K1** | 一个事实一个所有者：状态只在 `Interaction`；全仓无 `bool enabled_` / `bool pressed_` / `bool dragging_` 成员 | `grep -c "bool enabled_" src/ui → 0` |
| **K2** | 树的唯一走法：`children_` 私有，遍历只用 `for_each_child()` | `grep "children_" src/ui --include=*.hpp \| grep -v node/ → 0` |
| **K3** | 几何唯一写入口：`geometry_` 私有，写只用 `set_geometry()` | `grep "global_ = \|global_\.pos = \|global_\.size = " → 0` |
| **K4** | 语义是投影：`Semantics` 无 setter，字段全部 `const` 派生 | 编译期（无 `set_*`） |
| **K5** | 新控件不新增基类虚函数：R1 后基类虚函数 ≤ 34（可选 R7 后 ≤ 30） | `grep -c "    virtual " node/*.hpp` |
| **K6** | 测量缓存一套、key 可扩展 | 单一 `MeasureCache` 成员（protected）；`MeasureKey` = constraints + epoch + 派生输入（text_hash/font_size/wrap_width/…） |

补充不变量（沿用已有教训，写进测试）：

- **一帧一角色一状态**（UI-46）：同一控件同一帧对同一 `StyleRole` 只能用一种 `ControlState`。用调试断言 + 单测锁。
- **范围合法**（UI-34/UI-45）：`clear_layout_dirty` / `invalidate_measure` 必须经 `for_each_child` 递归到 overlays。

---

## 7. 43 个控件的分类与 `WidgetRole` 映射

| 分类 | 控件（数量） | `WidgetRole` |
|---|---|---|
| Leaf（19） | Button / Checkbox / Radio / Slider / Progress / Label / Image / ColorRect / PreviewRuler / Tooltip / Spacer / CollapsibleHeader / DragFloat / ColorInput / TextInput / TabBar / Scrim / **SceneView / TimelineView** | Button / CheckBox / Radio / Slider / Progress / Label / Image / Custom / Custom / Tooltip / Custom / Custom / TextField / TextField / TextField / Custom / Custom / Custom / Custom |
| Container（17） | LinearLayout / Hbox / Vbox / Grid / Wrap / Panel / ScrollView / ScrollPanel / Splitter / Section / TabView / Window / DialogHost / Dropdown / MenuBar / FileBrowser / RadioGroup | Panel（前 6 个）/ ScrollView / ScrollView / Splitter / Panel / Panel / Dialog / Custom / Menu / List / Radio |
| DataView（3，已有 `ListView` 模板方法） | ListView / Table / TreeView | List / Table / Tree |
| 浮层（3） | MenuPopup / MenuScrim / DropdownList | Menu / Custom / List |
| Root（1） | UIRoot | Custom |

> 计数：42 个由 `class … : public Widget\|LinearLayout\|ListView\|UIRoot\|Vbox\|Hbox\|ScrollView` 命中，
> 再加 `ScrollPanel`（经 `Panel` 继承）= **43**；上表 19+17+3+3+1 = 43。
> `SceneView` / `TimelineView` 不继承 `ListView`，但同样是“无子控件、自绘 N 行”，因此归 Leaf，绘制由 `BoxPainter`/`TextPainter` 分担。

> 说明：`Semantics.role` 是"给外部看这是什么"，不是分类学；`LeafNode/ContainerNode` 是"内部怎么实现"。两者不要混用。

---

## 8. 新控件的实现契约

| 场景 | 需要写 | 上限 |
|---|---|---|
| 静态图形 | `append_to()` + 构造期 `layout.preferred = …` | **~30 行** |
| 可交互叶子 | 同上 + `input.enabled` 检查 + 2~3 个 `on_*` | **~60 行** |
| 布局容器 | `measure()` + `arrange()`（+ 子项遍历走 `for_each_child`） | **~80 行** |
| 数据列表 | 继承 `ListView`，实现 `item_count()` + `draw_row()` | **~120 行** |

**禁止**：新增基类虚函数；在控件内手写 `control_state()`；在控件内手写焦点环；为部件做堆分配。

---

## 9. 与 `UISystem` / `UIRoot` 的边界（不变）

- 命中、焦点、手势、模态、DPI 继续在 `UISystem`（`ui_system.cpp` 558 行）。本次不拆它。
- `UIRoot` 继续拥有 `UIContext`（主题 + revision，UI-42 的成果保留）。
- 唯一变化：`UISystem` 写入的状态字段从"控件私有成员"改成 `input.hovered / input.focused`（API 不变：仍调 `set_hovered/set_focused`）。

---

## 10. 被否方案

| 方案 | 否因 |
|---|---|
| 6 个基类（Node / Container / LayoutNode / InputTarget / StyleState / Widget 各为一个基类） | C++ 里 = 6 个 vptr + 交叉转换；`InputTarget` 只被 `UISystem` 使用，用 `dynamic_cast` 定位反而更慢 |
| 部件用 `unique_ptr<Part>` | 每控件 4~5 次堆分配；与"内存要降"目标直接冲突 |
| Flutter 三段式 | 对象数 ×3；编辑器规模下无收益 |
| 约束求解（ConstraintLayout 式） | 全局耦合与现有局部脏标记/测量缓存冲突；表达力收益不需要 |
| 立即模式 | 与焦点/撤销/时间线/属性面板不兼容 |
| 一次性重写 `ui/` | 2,018 条断言归零；算法正确，只有接缝错 |

---

## 11. 变更记录

| 日期 | 变更 |
|---|---|
| 2026-11-27 | 首版：类型模型、六条契约、43 控件映射、目录与依赖规则 |
| 2026-11-27 | R1 实施后校正：「纯样板 arrange」实测 **17** 个（原估 20）；`CollapsibleHeader`/`TextInput`/`FileBrowser` 的 arrange 另含自己的语义，不属样板 |
| 2026-11-27 | R2 实施后校正：部件成员名 `layout` → `layout_`（与既有的虚函数 `layout()` 冲突）；`Display` 枚举下沉到 `layout_params.hpp` 并用 `using Display = ::xel::ui::Display;` 保持 `Widget::Display::Flow` 写法 |
| 2026-11-27 | R3 实施后记录：几何写绕过 12 处（估 11，漏算 menu_bar 构造的 `local_`）；`clear_children()` 保持 public；Section 改 `adopt(content_)`；越界改 `src/cosine/preview_pane.hpp`（shadow 了基类 `children_`） |
| 2026-11-27 | R4 实施后校正：不建 Theme 样式索引表（改为按需 `theme.style(role,state)` 现算，避免第二份真相）；槽内保留 `started`/`reversing` 两个 bool 以维持“首帧吸附 + 反向不跳变”；实测 `StyleSlot` 8 B、`VisualState` 104 B、`sizeof(Widget)` 384 B；`StyleRole`/`ControlState` 底层类型固定为 `uint8_t` |
| 2026-11-27 | R5 实施后记录：新增 `ui/measure_cache.hpp`（`MeasureKey` 56 B / `MeasureCache` 72 B）；文本键用 FNV-1a 哈希（不存 `std::string`，否则 Widget 456 B）；`Widget` 432 B；Label 删除自带缓存 45 行，TreeView 行缓存接入 `invalidate_measure()` |
| 2026-11-27 | R6 实施后记录：新增 `semantics/`（WidgetRole 20 种 + 派生投影，26 控件覆写）；8+3+5 份状态字段并入 `Interaction`；删除 `control_state()` 自由函数；新增 `test_ui_architecture`（D1–D3 + K1/K2/K3/K5/K6） |
| 2026-11-27 | R8（UI-54）include 重写：259 处改到新路径（22 条映射，前后计数均 259）；删除 22 个转发头，`src/ui` 根目录文件数归 0；D5 改为“根目录无头文件 + 无转发头”；修复遗漏的 `test/CMakeLists.txt` PCH 路径 |
| 2026-11-27 | R10（UI-55）`menu_bar.hpp` 867→562（拆出 menu_item/menu_popup）；语义钩子 5→3（checkable/editable 改 role 派生） |
| 2026-11-27 | R11（UI-56）**真分层落地**：`node/node.hpp`(291 行/17 虚) + `node/layout_node.hpp`(379 行/22 虚) + `container_node.hpp`；`widget.hpp` 退化为 17 行别名；`sizeof(Node)=112 < LayoutNode=432`；树元素类型改为 `Node*`；Node 声明布局契约纯虚以便无转型遍历；`LayoutNode::child_at` 协变隐藏 + `for_each_layout_child`；K8 断言锁住结构 |

---

## 12. 与旧文档的关系（不改旧文档）

| 旧文档 | 现状描述 | 本设计的差异 |
|---|---|---|
| `docs/ui_architecture.md` §一 | `Widget` = 身份 / parent / 几何 / 可见性 / 主题版本 | 实际还含测量缓存、输入钩子、样式过渡、盒模型、层叠、诊断（9 类）；本设计把它们分到 `Node/LayoutNode/Interaction/VisualState/BoxMetrics` |
| `docs/ui_architecture.md` §二.5「交互三布尔」 | `hit_testable/focusable/clips_children` | `clips_children` 降为派生函数（K5）；另加 `Interaction` 状态契约（K1） |
| `docs/ui_theme_tokens.md` | `StyleRole` 12 种 + `ControlState` 5 种 | **不变**（这是已成功的一层，本设计只把它的实现搬进 `VisualState`） |
| `docs/review/ui-widget-v1-*.md` | Widget 过重的量化诊断 | 本设计是它的实施蓝图；其中有 2 处计数需更正，见 `docs/todo-ui.md` 附 G |
