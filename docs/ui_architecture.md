# UI 层架构

> 范围：`src/ui/` 与它依赖的 `src/graphics/`。
> 本文只描述**当前有效**的结构与契约；欠账与待办见 `docs/todo.md`。

## 一、分层

```
┌─ graphics/ ──────────────────────────────────────────────────────┐
│ DrawList · DrawBatch · Vertex   画什么（顶点、槽、clip、opacity）│
│ box / rectangle / line / sprite / triangle   图元                │
│ font_atlas（字形栅格化 + 度量 + 排版辅助）                       │
└────────────────────────────┬─────────────────────────────────────┘
                             │ append_to(DrawList&)
┌─ ui/ ──────────────────────▼─────────────────────────────────┐
│ Widget        身份 / parent / 几何 / 可见性 / 主题版本       │
│ 布局容器       Vbox Hbox Grid Wrap Panel Splitter ScrollView │
│               ScrollPanel UIRoot                             │
│ 叶子控件       Label Button Checkbox Radio Slider Progress   │
│               TextInput Dropdown Image Rect                  │
│ 复合控件       MenuBar TabBar Table TreeView TimelineView    │
│               SceneView FileBrowser Window Section Dialog    │
│               CollapsibleHeader PreviewRuler DragFloat       │
│               ColorInput Scrim Tooltip                       │
│ 行为基元       ScrollState ListView                          │
│ 调度          UISystem（命中 / 焦点 / 布局触发 / 手势）      │
└────────────────────────────┬─────────────────────────────────┘
                             │ 提交 DrawList
┌─ backend/vulkan/ ──────────▼───────────────────────────────┐
│ renderer：多纹理槽（16）、离屏渲染 + PNG 读回              │
└────────────────────────────────────────────────────────────┘
```

依赖是单向的：`graphics` 不知道 `ui`，`ui` 不直接调用 Vulkan（只产出 `DrawList`）。

### 控件间依赖

按 `#include` 关系归纳。`theme` / `style` / `widget` 几乎被所有控件引用，下表中省略。

**基类链**

```
Drawable (graphics) 
  └─ Widget ─┬─ Vbox ─── Panel ─── ScrollPanel
             ├─ Hbox / Grid / Wrap
             ├─ Splitter / ScrollView / UIRoot / Scrim
             ├─ Label / Button / Image / ...（叶子）
             ├─ ListView ─── Table
             └─ Section / Window / Dialog / MenuBar / TabView / TreeView / ...（复合）
```

**叶子控件：纹理与占位**

`Image`（`src/ui/core/image.hpp`）有两种绘制路径：

- `texture_id == 0`：占位（圆角底色 + 边框 + 对角交叉 + 文案）。
- `texture_id != 0`：纹理。`texture_id` 语义为「图片槽索引 + 1」，与 `scene::Node::texture_id`、
  `editor::TextureCache` 一致；`slot = graphics::image_slot(texture_id - 1)`。
  `Fit` 由 `compute_placement()` 转成目标矩形与 UV：`Fill` 拉伸、`Contain` 内接居中、`Cover` 填满并用
  `DrawList::push_clip` 裁剪；纹理尺寸未知时回退 `Fill`。
- 纹理模式为直角，可选底板（`set_background`）与边框（`set_border_visible`）；`Image` 不负责解码、
  上传与槽分配。

**组合与持有关系**（谁内部装了谁）

| 控件 | 内部依赖 | 说明 |
|---|---|---|
| `Panel` | `Vbox` | 继承 Vbox，加 padding + Surface 背景 |
| `ScrollPanel` | `Panel` + `ScrollView` + `Vbox` | 面板 + 可滚动内容 |
| `Section` | `Vbox`（内容）+ `CollapsibleHeader`（标题条） | header 不在 `children_` 里，`arrange` 时手动摆并手动传 context |
| `Window` | `Vbox`（`body_`）+ `UIRoot`（找根做模态） | `body_` 走 `adopt()` 挂入，否则冒泡链会断 |
| `Table` | `ListView` | 继承，覆写 `viewport_height()` 扣除表头 |
| `FileBrowser` | `Table` | 持有 `table_`；事件自己算命中，选中同步给它 |
| `Dialog` | `Button` `Hbox` `Label` `Panel` `UIRoot` `Scrim` `Vbox` | 组合最多的一类 |
| `MenuBar` | `UIRoot` | 需 `add_overlay` 挂弹出菜单 |
| `Dropdown` | `UIRoot` | 同上（选项列表是浮层） |
| `TabView` | `TabBar` | 标签条 + 内容页 |
| `RadioGroup` | `Vbox` + `Radio` | 纵向排列互斥项 |
| `DebugOverlay` | 13 个控件 | 最大的组合者，仅示例使用 |

**基础依赖分层**

| 层 | 头文件 | 依赖 |
|---|---|---|
| L0 | `align` / `rect` / `scroll_mode` / `theme` | 无 |
| L1 | `style` / `uicontext` | `theme` |
| L2 | `widget` | `rect` `align` `uicontext` `style` |
| L3 | `vbox` `hbox` `grid` `wrap` `root` `scrim` `scroll_state` | `widget`（`scroll_state` 只依赖 `scroll_mode`） |
| L4 | `panel` `scroll_view` `list_view` `collapsible_header` + 各叶子控件 | L3 |
| L5 | `table` `section` `window` `dialog` `menu_bar` `tab_view` `file_browser` | L4 |

**两条约束**

1. **`graphics` 不认识 `ui`** —— 单向依赖；`ui` 只产出 `DrawList`，不直接调 Vulkan
2. **需浮层的控件依赖 `UIRoot`**（`MenuBar` / `Dropdown` / `Dialog`）—— 为拿 `add_overlay`，不是类型继承关系

## 二、五个核心契约

架构升级（UI-7 阶段 1–5）确立的全部显式契约。新增控件时按这些接口判断行为，不要靠约定。

### 1. 子项挂载：`adopt()`

```cpp
void adopt(std::unique_ptr<Widget> child);   // Widget，protected
std::vector<std::unique_ptr<Widget>> children_;   // 唯一存储
```

- **唯一合法入口**。它做三件事：`set_parent` / `set_context` / 入 `children_` / `mark_layout_dirty`
- 派生类**不要**直接 `children_.push_back`——漏设 `parent` 会让事件冒泡链断掉（历史上导致「窗口内容区拖不动」）
- 验收点：`grep -rn "children_.push_back" src/ui/` 只应命中 `Widget::adopt` 内部

### 2. 布局测量：`measure()` + `fixed_size()`

```cpp
virtual glm::vec2 measure(glm::vec2 constraints) = 0;
virtual bool fixed_size() const { return false; }
```

- `constraints.x` 是父容器给的**可用宽度**，返回值宽度不得超过它（`min_size` 强制除外）
- `fixed_size() = true` 表示忽略 `constraints`，尺寸由 `min/max` 决定（`Table`、`FileBrowser`）
- `invalidate_measure()` 只失效本控件的缓存；`invalidate_measure_recursive()` 才递归（主题切换等全树变更用）

**子项测量的唯一入口**（UI-31 起）：

```cpp
static glm::vec2 Widget::measure_child(Widget& child, glm::vec2 content);
```

- `content` 是父容器**内容区**的可用尺寸（已扣 padding，**未扣 margin**）
- `margin` 在函数内部全轴扣除；`size_percent` 在扣完 margin 的可用尺寸上计算
- 轴无界（`!is_bounded(v)`）时百分比不生效；无界值不能减 margin（否则 1e8 减去后会被当成"有界"）
- 顺序固定：`margin → pct/fill → measure → aspect_ratio + clamp（一起求解）`
- 宿主（`UIRoot::layout_child`、`TabView`、`ScrollPanel`、`DialogHost`、`Splitter`）一律走这个入口，
  不允许自己直接调 `child->measure()`

**主轴预算**（UI-32 起，`src/ui/layout_budget.hpp`）：

```cpp
struct MainAxisItem { pct; fill; fixed; flex; natural; margin_before/after; min_size; max_size; };
std::vector<float> resolve_main_axis(float available, float spacing,
                                     const std::vector<MainAxisItem>& items,
                                     bool distribute_flex = true);
```

- `Vbox`（主轴 y）/ `Hbox`（主轴 x）共用一个解算器，不再各写一份 extra 分配
- **预算先于 flex**：`pct` / `fill` 先认领主轴空间，`fill` 等价于 `pct = 1`；
  多个预算子项超出可用空间时按认领额等比压缩，保证总和 ≤ 可用（旧实现每个 pct 各拿整高）
- 预算按 `pct × (available − 自身主轴 margin)` 计价，所以 `pct + margin` 不再溢出
- 非预算子项取自然尺寸；`flex_grow > 0` 的子项主轴自然尺寸取 `min_size`（不取内容尺寸），
  否则 `ScrollPanel` 这类内容远大于可视区的控件会把容器撑爆
- 剩余空间再按 `flex` 分配：`extra > 0` 增长（上限 `max_size`）、`extra < 0` 收缩（下限 `min_size`），
  迭代到边界后剩余归零
- `measure()` 用 `distribute_flex = false`：容器自然尺寸不含 flex 撑开；`arrange()` 才分配
- 无界主轴直接返回自然尺寸（clamp），不做预算与 flex 分配

**stretch 与尺寸规格的互斥规则**：

| 子项声明 | 交叉轴 stretch | 说明 |
|---|---|---|
| `size_percent`（该轴 > 0） | 跳过 | 百分比已显式定了尺寸 |
| `fill_x/fill_y` | 跳过 | Fill 就是"拉满"的显式写法 |
| `fixed_size()` | 跳过 | 固定尺寸控件不吃父级拉伸 |
| 其它 | 生效 | 拉满后**重新** `clamp_size` |

- `aspect_ratio` 与 `clamp` 一起求解（`apply_aspect_clamp`）：任一轴被 `min/max` 截断后回算另一轴，
  不再出现"先算比例再 clamp 把比例破坏"；stretch 的结果同样走这一步

**叶子的自然尺寸**（UI-32 起）：

```cpp
void Widget::set_preferred_size(glm::vec2);   // 构造期确定的自然尺寸
glm::vec2 Widget::preferred_size() const;
```

- `measure()` 读 `preferred_size()`，**不读** `local_.size` / `local_bounds().size`
  （后者是最近一次 `arrange` 的结果，被 stretch 放大后会让控件"只涨不缩"）
- 已迁移：`ColorRect` / `Image` / `ListView` / `SceneView` / `TimelineView` / `TreeView` /
  `CollapsibleHeader`（按标题文字算）/ `DropdownList` / `MenuPopup`
- 浮层（`DropdownList` / `MenuPopup`）在内容变化时同步 `set_preferred_size`，
  这样 root 重排时不会把它们按构造尺寸摆放
- 例外：`Table` / `FileBrowser` / `Window` 走 `fixed_size()` + `min/max`，语义不变

**measure 缓存与脏标记**（UI-34 起）：

```cpp
std::uint64_t layout_epoch();          // UIRoot::layout() 每轮 +1
bool    Widget::measure_cache_hit(constraints);   // 有效 && 同轮 && 约束相同
glm::vec2 Widget::measure_cached();
void    Widget::store_measure(constraints, result);
void    Widget::mark_measured();       // 本轮已统计（测过或用规则跳过），清“已通知”标记
void    Widget::invalidate_measure_recursive();   // set_theme / 密度 / 字号切换用
```

- 缓存键是**（约束，布局轮次）**：只在同一轮内命中，跨轮必然重算——不会读到旧值，
  但同轮内 `parent.measure()` 与 `parent.arrange()` 共享一次测量（文字测量从每控件 2 次降到 1 次）
- 缓存统一在 `Widget` 里（子类不再各持三元组）；`measure_child()` 也走同一缓存，
  所以嵌套容器的重复测量同样被合并
- `UIRoot::set_theme()` 递归 `invalidate_measure()`：主题/密度/字号切换后下一轮数值立即一致
- `arrange()` 一律用 `set_geometry()` 写几何（不再 `set_pos/set_size`）：布局过程不制造脏状态、不清缓存
- `mark_layout_dirty()` 早退：以“本次变更新鲜度”标记 `notified_` 为判据（`invalidate_measure()` 之后若已通知过祖先就直接返回）；
  `UIRoot::layout()` 收尾 `clear_layout_dirty_recursive()` 同时清 `layout_dirty_` 与 `notified_`
- 验收点：`test_ui_layout_probe`（缓存命中 / 主题刷新 / 文字测量基线）、`test_ui_theme_relayout`（密度循环 / 字号 / 明暗主题一致性）

**可见性三态**（UI-33 起）:

```cpp
enum class Display { Flow, Positioned, Hidden };
Display display() const;                 // visible_ 为假时恒为 Hidden
void    set_display(Display);
set_visible(false)  == Hidden             // 不占位、不绘制、不参与命中
set_absolute(true)  == Positioned         // 不占流式空间，但仍 arrange（absolute_pos）
```

- 四个流式容器（`Vbox`/`Hbox`/`Grid`/`Wrap`）在 measure/arrange 里只处理 `Flow`，
  随后单独摆 `Positioned`；`Hidden` 完全跳过
- `UIRoot::layout_child` 也跳过 `Hidden`（保留旧几何，重新可见会重排）
- `TabView` 的页切换继续用 `set_visible`，但 `measure` 只算当前页，因此切页不会让尺寸漂移

**溢出策略**（UI-33 起）:

```cpp
enum class Overflow { Visible, Clip, Scroll };
virtual Overflow overflow() const;               // 默认 Visible
virtual bool clips_children() const;             // = overflow() != Visible（唯一实现）
glm::vec2 content_extent() const;                // 容器 arrange 时记录的内容尺寸
bool content_overflows() const;                  // 内容 > 自身尺寸（任意轴）
```

| 控件 | overflow |
|---|---|
| `Panel` / `TabView` / `Window` / `Splitter` / `Table` / `TreeView` | `Clip` |
| `ScrollView` | `Scroll`（同样裁剪，但自带滚动条） |
| 其它（含四个流式容器） | `Visible` |

- `clips_children()` 不再由各控件覆写，只由 `overflow()` 派生，避免绘制/命中口径分叉
- `content_overflows()` 用于“内容放不下”的诊断与后续布局调试面板（T30）
- `Splitter`：`measure()` 由两个 pane 的自然尺寸推导；`arrange()` 分配像素时两侧都
  尊重子项 `min_size`（`clamp_first`），不再把一侧挤成负数

**几何写入**（UI-31 起）：

```cpp
void Widget::set_geometry(Rect slot);      // arrange 内使用：local_ 与 global_ 同时写，不触发脏标记
void Widget::set_arranged_pos(glm::vec2);  // 固定尺寸控件（Window）只跟随位置
```

- `arrange()` 只能用这两个入口写几何；`set_pos/set_size` 代表"外部请求"，会把树标脏
- 容器必须写 `local_`：`local_bounds()` 等于最近一次 `arrange` 的 slot（DebugWindow/调试面板依赖这一点）
- 容器 `layout()` 用 `global_` 重排，不能用 `local_`（陈旧值会把布局打回构造尺寸）
- 整轮 `UIRoot::layout()` 处于 `LayoutPassScope` 内：期间的脏标记只失效缓存、不重新标脏，
  所以布局结束时 `root->layout_dirty()` 为 false

**无界约束**：

```cpp
ui::kUnbounded          // 1e8；constraints 轴 >= 它即无界
ui::is_bounded(float)   // [0, 1e8) 为有界；负值（历史上的 -1）也算无界
```

### 3. 层级：`fills_parent()` + `Layer`
```cpp
virtual bool fills_parent() const { return false; }
enum class Layer { Content, Floating, Modal };
virtual Layer layer() const { return Layer::Content; }
```

- `fills_parent()`：不参与测量协商，直接铺满父容器（`Scrim`）
- `Layer` 是跨 `children_` 与 `overlays_` 的**唯一主排序键**，`z_index` 只在同一 layer 内生效。

**排序契约**（`UIRoot::layer_paint_order()` 是唯一消费入口）：

```
Content < Floating < Modal
绘制 append_to：    layer 升序 → z 升序（稳定，同值按加入顺序）
命中 collect_hit_path：layer 降序 → z 降序
Tab collect_focusable：layer 升序 → z 升序
```

- `overlays_` 保留 API，但**不再是一等顺序来源**，只作为 `layer_paint_order()` 的输入之一。
- 典型归属：普通子项与控件默认 `Content`；`Window`（模态）、`Scrim` 与 `DialogHost` 为 `Modal`；
  `MenuPopup` / `MenuScrim` / `DropdownList` / `Tooltip` 为 `Floating`。
- `DialogHost` 虽是 overlay 宿主，但它经 `push_modal(host, with_scrim=false)` 进入模态栈（有键盘/焦点隔离），
  层级也必须是 `Modal`，否则会被模态 `Window`（同为 Modal 层）盖住。
- 模态帧的 z 在 `UIRoot::layout()` 归一化：frame `i` → scrim `2i+1`、window `2i+2`，保证嵌套模态
  不会因 `kOnTopZ` 撞 z 而靠插入顺序兜底。

### 4. 视口：`viewport_height()`

```cpp
virtual float viewport_height() const { return global_.size.y; }
```

- `ListView` 的滚动/可见行计算都用它
- `Table` 覆写为 `body_height()` 以扣除表头——这是「带表头的列表」接入 `ListView` 的关键钩子

### 5. 交互三布尔

| 布尔 | 语义 |
|---|---|
| `hit_testable() = true` | 命中到此为止，`path` 末端是它自己（可点、可拖） |
| `hit_testable() = false` | 继续向下找子项；子项都没命中时它也不接（`Vbox` 用这个让点击冒泡） |
| `overflow() = Clip/Scroll` | 子项绘制/命中裁剪到自身 `bounds`（`Panel` / `Window` / `ScrollView` / `Table` / `TreeView` / `Splitter`） |
| `focusable() = true` | 参与 Tab 顺序，可被 `set_focus` 获得键盘事件 |

### 6. 内边距：显式值优先，主题只填空白

```cpp
void set_padding(float all);          // 显式意图：锁定，主题重刷不再覆盖
void apply_theme_padding(float all);  // 主题套用入口：跳过已锁定的控件
bool padding_override() const;        // 查询是否已锁定
void clear_padding_override();        // 交还主题
void set_spacing(float spacing);      // UI-35：间距同语义（锁定）
void apply_theme_spacing(float s);    // 主题套用入口
bool spacing_override() const;
```

- `Vbox` / `Hbox` 构造期的 4px 是**结构性默认值**（`set_default_padding`），不锁定；
  所以没显式设过的 `Panel` / `TabView` / **裸容器**照旧跟着 `theme.padding` 走。
- **裸容器也接主题**（UI-35）：`Vbox` / `Hbox` / `Grid` / `Wrap` 的 `set_context` 会
  `apply_theme_padding` + `apply_theme_spacing`（定义在 .cpp，避开头文件拖入 UIContext）。
  显式 `set_padding(0)` / `set_spacing(0)` 仍然优先，主题切换/密度切换不漂移。
- `Panel` / `TabView` 的 `set_context` 走 `apply_theme_padding`：既要「主题能注入」，
  又不能把调用方写死的 0 冲掉。`UIRoot::set_theme` 会全树 `set_context`，这是唯一被重刷的时机。
- 典型用法：窗口内容页要贴满时 `panel->set_padding(0.0f)` 一次即可，之后切 density / 明暗仍为 0。
- 相关：`TabView::set_tab_gap(0)` 去掉标签栏与页面之间的 `theme.spacing` 缝（负值 = 跟随主题）。
- 验收点：`test_ui_context`（Panel/Vbox 锁定 + 裸容器跟随）、`test_ui_tab_view`（tab gap 与 `set_theme` 后不漂移）。

### 7. 布局内核与工具（UI-36）

```cpp
// src/ui/containers/linear_layout.hpp
class LinearLayout : public Widget { enum class Axis { Vertical, Horizontal }; ... };
class Vbox : public LinearLayout;   // Axis::Vertical，cross_align 默认 top_center
class Hbox : public LinearLayout;   // Axis::Horizontal，cross_align 默认 center_left

// src/ui/layout_utils.hpp
Rect content_rect(const Widget&, Rect slot);                       // 扣 padding 的内容区
void place_positioned_children(Widget&, Rect content, vec2 cons);  // 摆 Positioned 子项
```

- 主轴解算、margin、cross 轴 stretch、主轴对齐（Start/Center/End/SpaceBetween）、
  绝对定位子项全部只写一次；`Vbox`/`Hbox` 只剩“选轴 + 交叉轴默认值”
- `Grid`/`Wrap`/`LinearLayout` 的绝对定位子项摆放共用 `layout::place_positioned_children`
- 尺寸/对齐规格快照（不新增存储，只是转发）：

```cpp
Widget::LayoutSpec spec = w.layout_spec();   // align / flex / pct / aspect / fill / min / max
w.set_layout_spec(spec);                      // 回写，等价于逐个 setter
```

- **基线对齐**：`Align::baseline_align()` + `Widget::baseline_offset_for(h)`（Label/Button 覆写）。
  Hbox 先算行内最大基线，再把子项对齐到它；Settings 的“标签 + 控件”行已改用基线对齐
- **布局调试面板**：DebugWindow 第三页 `Layout`（只读）：global/local bounds、临时 measure、
  display/overflow/overflows/dirty/measure_valid/baseline、content extent、clips、LayoutSpec。
  验证钩子：`COSINE_DEBUG_TAB=layout`、`COSINE_DEBUG_TARGET=1`
- `content_extent` 只由 `arrange` 写入（子项包围盒），是 `content_overflows()` 的诊断依据；
  `measure()` 不得污染它

## 三、生命周期

### 构建 → 布局 → 绘制

```
add/adopt  ──▶  measure(constraints)  ──▶  arrange(slot)  ──▶  layout()  ──▶  append_to(DrawList&)
                 缓存结果                设置 global_         排子项        写顶点
```

- `mark_layout_dirty()` 沿 `parent` 链传播到 `UIRoot`；已通知过的子树会早退（UI-34，见上方“measure 缓存与脏标记”）
- `UISystem::tick()` 里：`if (resized || root_->layout_dirty()) { root_->layout(); root_->clear_layout_dirty(); }`
- **自由画布语义**：`UIRoot::layout_child` 用 `child.x()/y()`（即 `local_.pos`）定位，所以绝对定位控件必须**同时**维护 `local_.pos` 与 `absolute_pos`
- `fills_parent()` 的控件跳过协商，直接 `arrange({parent.pos, parent.size})`
- **顶层 Fill 模式**（UI-35）：同时声明 `fill_x && fill_y` 的顶层子项（应用的 `chrome_` Vbox）直接铺满 root；
  窗口/弹层等其余子项仍走自由画布定位。editor 因此不再手写 `place(...)`，也不再在 tick 里比较 `last_w_/last_h_`。

```
chrome_ (Vbox, padding 0, spacing 0, cross_align = h_stretch, fill_x+fill_y)
  ├─ menu_row (Panel, 高 = kMenuHeight)      └─ menu (MenuBar, absolute + fill)
  ├─ toolbar_row (Panel, 高 = kToolbarHeight)└─ toolbar (Hbox, absolute + fill)
  ├─ root_splitter (Splitter, flex_grow 1, h_stretch)
  └─ status_row (Panel, 高 = kStatusHeight)  └─ status (Label, absolute + fill)
```

### 事件

```
dispatch(InputEvent)
   └─▶ collect_hit_path(root)
         ├─ overlay 优先（按 z 降序）
         ├─ 然后 children（按 z 降序）
         └─ 尊重 clips_children 与 hit_testable
   └─▶ pressed_ = path.back()
   └─▶ 冒泡：bubble_to_root(pressed_) → 逐层 on_mouse_*，返回 true 即停
```

- 拖动依赖 `pressed_`：按下时定一个目标，后续 `move` 冒泡给它
- 手势阈值在 `UISystem`：`double_click_ms` / `long_press_ms` / `double_click_slop`

## 四、行为基元

不给基类加职责，而是做成可持有的小类型。

| 基元 | 职责 | 使用者 |
|---|---|---|
| `ScrollState` | 一维滚动：值/目标/平滑（指数衰减）+ Fast 加速 + snap + range 夹取 | `Table` / `TreeView` |
| `ScrollView` | 二维滚动容器；`measure()` 返回控件自身尺寸，`viewport_size()` 才是扣除滚动条后的内容区 | `ScrollPanel` |
| `ListView` | 行列表：滚动/选中/键盘/可见行窗口/像素对齐/行循环/滚动条；子类只实现 `item_count()` + `draw_row()` | `Table`（`FileBrowser` 经 Table 受益）、`TreeView` |
| `CollapsibleHeader` | 折叠标题条：`panel_bg` 底 + 三角 + 标题 | `Section` |
| `Scrim` | 全屏遮罩（alpha/layer 可配），吞掉指针/滚轮/点击/右键/双击/长按，不参与 Tab | `UIRoot` 模态栈、下拉 |
| `FontAtlas::centered_text_y()` | 行框在给定高度内垂直居中 | 所有自绘文字 |
| `FontAtlas::estimate_text_width()` | 字符数 × 字号 × 0.5 的宽度估算（无 atlas 回退） | 各控件的文本估测 |

## 五、模态窗口

`Window::set_modal(true)` 的链路：

```
Window::set_modal
   └─▶ UIRoot::push_modal(this)
          ├─ 造一个 Scrim 放进 children_（Modal 层）
          ├─ 快照当前焦点 → ModalFrame.prev_focus
          ├─ 初始焦点：焦点已在模态子树内则保留；否则 preferred_focus() → 首个可聚焦 → 窗口自身
          └─ 记入 modal_frames_（单一 ModalFrame{window, scrim, prev_focus} 栈，支持嵌套）
UIRoot::layout() 归一化模态帧 z：frame i → scrim 2i+1 / window 2i+2
Window::close()/destroy()/set_modal(false) → release_modal() → pop_modal()
```

- 遮罩 z 比窗口小 1：命中时窗口先命中，绘制时遮罩先画
- 模态栈合并为 `std::vector<ModalFrame>`：`window` / `scrim` / `prev_focus` 三份状态在一个元素里，
  `push_modal` / `pop_modal` / `layout()` 悬空剔除三处各维护一次，不再有「三数组错位」风险
- `UISystem::focus_next` 在有模态时把收集范围收窄到 `modal_top()`
- `dispatch_key` / `dispatch_char` 在模态打开时只作用于最上层模态子树；焦点在作用域外则忽略
- `pop_modal` 恢复打开前的焦点快照（仍在树中且不在被弹出子树内才恢复，否则清空）
- `DialogHost`（overlay 宿主）经 `push_modal(host, with_scrim=false)` 纳入模态作用域，自带遮罩、层级为 Modal
- 解除路径统一走 `release_modal()`，避免「遮罩残留、界面永久不可点」；窗口被直接销毁时 `layout()` 剔除悬空帧

## 六、主题

- 颜色按 **sRGB 直觉值书写**（`srgb_hex(0xRRGGBB)`），**存储为线性**；渲染目标是 `*_SRGB`，硬件做编码
- 强调色拆两个：`accent`（填充/边框/焦点环）与 `accent_text`（当文字，对比度更高）
- `StyleRole` + `ControlState` 提供「语义样式」，`Theme` 的裸颜色字段仍可直接读；两者并存，对照表见 UI-11
- **布局字面量收口**（UI-35）：原先散在各控件里的魔法数字进了 `Theme`，并随 `with_density` 缩放——
  `dialog_margin / dialog_min_width / dialog_min_height`（DialogHost）、
  `window_min_width / window_min_extra_height / window_keep_visible`（Window）、
  `splitter_handle`（Splitter 手柄）、`section_inset`（Section 内容内缩）。

## 七、绘制

- 控件实现 `append_to(DrawList&)`，只写顶点，不做提交
- `DrawList` 维护 clip 栈、纹理槽、透明度栈；变更会触发新 `DrawBatch`
- `DrawBatch::add_vertex()` 在写入时应用 `opacity`（预乘 alpha，四通道同乘即整体透明）
- 离屏导出：`set_size` + `layout()` + `append_to` + `update_font_atlas_texture()`，格式 `B8G8R8A8_SRGB`

## 八、已知边界

| 项 | 说明 | 记录位置 |
|---|---|---|
| 圆角无抗锯齿 | 几何近似，硬边 | `todo.md` D01 |
| 无外阴影 | 窗口层次靠 1px 边框 + `elevated` 令牌 | UI-6 遗留 |

## 九、编辑器层：ResourceBrowser

`src/cosine/resource_browser.hpp/.cpp` 是 cosine 专用的资源浏览器，不属于 `src/ui` 通用控件：

- 依赖 `editor::TextureCache`、`backend::vulkan::Renderer` 与 `stbi_info`，因此放在 cosine 层。
- 内部创建独立 `ui::Window("Assets")`，布局为：顶部地址栏 / 中间三栏 / 底部 footer。
- 三栏：分类固定 110、`FileBrowser` 弹性填充、预览固定 240。
- 顶部：Up + 路径输入 + 搜索 + Go；底部：选中摘要 + Import。
- 双击文件不导入；Import 按钮通过 `set_on_import` 交给 `Editor::import_image`。
- 预览纹理走 `TextureCache`，14 个图片槽满时回退占位；与项目导入共享同一缓存。
- 入口：`Editor` 的 `View → Assets`；打开/保存项目后自动切到项目目录。

### SettingsWindow

`src/cosine/settings_window.*`：左侧分类按钮（General / Preview / Export / About）+ 右侧 `ScrollPanel`。

- General：Rounded style、Light theme、UI text size，变更后 `apply_settings()` 即时生效。
- Preview：背景色与标尺显隐即时；画布尺寸与 Fit 存值，待预览缩放任务消费。
- Export：宽/高/FPS/背景/输出目录写入 `EditorSettings`，供 CO-3 使用。
- About：版本、平台、编译器、构建类型、当前项目路径（只读）。
- 入口：`Editor` 的 `View → Settings...`。
| 无性能测量手段 | 优化前需先有统计 | UI-15 |
