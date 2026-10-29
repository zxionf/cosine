# UI 层架构

> 范围：`src/ui/` 与它依赖的 `src/graphics/`。
> 本文只描述**当前有效**的结构与契约；欠账与取舍见 `docs/tech_debt.md`，演进计划见 `.design/lines/ui.md`。

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
- `invalidate_measure()` 会沿 `parent` 链向上传播；已失效则不再重复向上

### 3. 层级：`fills_parent()` + `Layer`

```cpp
virtual bool fills_parent() const { return false; }
enum class Layer { Content, Floating, Modal };
virtual Layer layer() const { return Layer::Content; }
```

- `fills_parent()`：不参与测量协商，直接铺满父容器（`Scrim`）
- `Layer` 目前**只是声明**，命中顺序仍沿用「overlay 优先 + children 按 z 降序」

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
| `clips_children() = true` | 子项绘制/命中裁剪到自身 `bounds`（`Panel` / `ScrollView` / `Table`） |
| `focusable() = true` | 参与 Tab 顺序，可被 `set_focus` 获得键盘事件 |

## 三、生命周期

### 构建 → 布局 → 绘制

```
add/adopt  ──▶  measure(constraints)  ──▶  arrange(slot)  ──▶  layout()  ──▶  append_to(DrawList&)
                 缓存结果                设置 global_         排子项        写顶点
```

- `mark_layout_dirty()` 沿 `parent` 链传播到 `UIRoot`
- `UISystem::tick()` 里：`if (resized || root_->layout_dirty()) { root_->layout(); root_->clear_layout_dirty(); }`
- **自由画布语义**：`UIRoot::layout_child` 用 `child.x()/y()`（即 `local_.pos`）定位，所以绝对定位控件必须**同时**维护 `local_.pos` 与 `absolute_pos`
- `fills_parent()` 的控件跳过协商，直接 `arrange({parent.pos, parent.size})`

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
| `ScrollState` | 一维滚动：值/目标/平滑（指数衰减）+ Fast 加速 + snap + range 夹取 | `Table` |
| `ListView` | 行列表：滚动/选中/键盘/可见行窗口；子类只实现 `item_count()` + `draw_row()` | `Table`（`FileBrowser` 经 Table 受益） |
| `CollapsibleHeader` | 折叠标题条：`panel_bg` 底 + 三角 + 标题 | `Section` |
| `Scrim` | 全屏半透明遮罩，吃指针/滚轮，不参与 Tab | `UIRoot` 模态栈 |
| `FontAtlas::centered_text_y()` | 行框在给定高度内垂直居中 | 所有自绘文字 |

## 五、模态窗口

`Window::set_modal(true)` 的链路：

```
Window::set_modal
   └─▶ UIRoot::push_modal(this)
          ├─ 造一个 Scrim 放进 children_，z = 窗口 z − 1
          └─ 记入 modals_ / modal_scrims_（栈，支持嵌套）
UIRoot::layout() 每帧把 scrim 的 z 重设为「窗口 z − 1」（raise 归一化后也不会错位）
Window::close()/destroy()/set_modal(false) → release_modal() → pop_modal()
```

- 遮罩 z 比窗口小 1：命中时窗口先命中，绘制时遮罩先画
- `UISystem::focus_next` 在有模态时把收集范围收窄到 `modal_top()`
- 解除路径统一走 `release_modal()`，避免「遮罩残留、界面永久不可点」

## 六、主题

- 颜色按 **sRGB 直觉值书写**（`srgb_hex(0xRRGGBB)`），**存储为线性**；渲染目标是 `*_SRGB`，硬件做编码
- 强调色拆两个：`accent`（填充/边框/焦点环）与 `accent_text`（当文字，对比度更高）
- `StyleRole` + `ControlState` 提供「语义样式」，`Theme` 的裸颜色字段仍可直接读；两者并存，对照表见 UI-11

## 七、绘制

- 控件实现 `append_to(DrawList&)`，只写顶点，不做提交
- `DrawList` 维护 clip 栈、纹理槽、透明度栈；变更会触发新 `DrawBatch`
- `DrawBatch::add_vertex()` 在写入时应用 `opacity`（预乘 alpha，四通道同乘即整体透明）
- 离屏导出：`set_size` + `layout()` + `append_to` + `update_font_atlas_texture()`，格式 `B8G8R8A8_SRGB`

## 八、已知边界

| 项 | 说明 | 记录位置 |
|---|---|---|
| 圆角无抗锯齿 | 几何近似，硬边 | `tech_debt.md` D01 |
| 无外阴影 | 窗口层次靠 1px 边框 + `elevated` 令牌 | UI-6 遗留 |
| `ListView::draw_row` 未被 `Table` 使用 | 行绘制仍在 `append_to` | UI-10 |
| `Layer` 未驱动命中排序 | 现有 overlay + z 已够用 | `tech_debt.md` T04 |
| 无性能测量手段 | 优化前需先有统计 | UI-15 |
