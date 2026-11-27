# src/ui 改造计划（todo）

> 版本：v1（2026-11-27 15:17 CST）
> 基线：main@167a3f83（工作区另有 `src/ui/widget.hpp` 未提交改动：`x/y/width/height` 去 `virtual`）
> 设计依据：`.design/ui-redesign.md`（v1）
> 范围：**只改 `src/ui/`**（+ 必要的 `test/`、`docs/` 引用）。不动 `src/anim`、`src/cosine`、`src/backend`。
> 编号：R1–R6 与设计文档 K1–K6 契约对应，可直接转成 sele 的 task/step。

---

## 0. 使用说明

- 每批（R1–R6）对应一个 task；批内每步对应一个 step。
- **顺序不可乱**：R1 → R2 → R3 → R4 → R5 → R6。每批结束必须 74 测试全绿 + 快照哈希不变（R4 之后允许变化并显式记录）。
- 每批独立可发布；不做跨批的"半成品"提交。
- R1–R3 **零行为变化**，快照哈希必须逐字不变（`c8a0966c` / `eb32c524`）——这是最硬的护栏。
- 每批完成时更新一次指标看板（§1）。

---

## 1. 指标看板

| # | 指标 | 现状 | 目标 | 验收命令（只读） |
|---|---|---|---|---|
| M1 | 单文件最大行数 | `widget.hpp` 630 | ≤ 250 | `wc -l src/ui/node/*.hpp \| sort -n \| tail -1` |
| M2 | 基类虚函数数 | 50 | R1 后 34；R6 后 ≤ 30 | `grep -c "    virtual " src/ui/widget.hpp`（R6 后改为 `node/*.hpp` 求和） |
| M3 | 纯样板 `arrange()` | 20 | 0 | 见附 B 的探测命令 |
| M4 | 状态字段副本 | `enabled_` 8 / `pressed_` 3 / `dragging_` 5 / `selected_` 6+1 | 各 1 | `grep -rc "bool enabled_" src/ui` |
| M5 | 测量缓存套数 | 3 | 1 | `grep -rln "measure_cache_hit\|MeasureKey\|cached_visible_rows" src/ui` |
| M6 | 过渡状态内存 | 1,688 B + 1 new | ≤ 128 B、0 new | 单测 `sizeof(VisualState)` 断言 |
| M7 | 几何写绕过点 | 11 | 0 | 见附 C 的探测命令 |
| M8 | 树遍历方式 | 2 种 / 4 点 | 1 种（`for_each_child`） | `grep -rn "children_" src/ui --include=*.hpp` |
| M9 | 新增控件行数 | 90~130 | ≤ 60 | 以新增的 `Tag`（示例控件）实测 |
| M10 | 测试数 / 断言数 | 74 / 2,018 | 不减少，新增 ≥ 40 断言 | `ctest` + `grep -rc "assert(" test` |

---

## 2. 批次总览

| 批 | 标题 | 目标 | 破坏性 | 预估 | 前置 |
|---|---|---|---|---|---|
| **R1** | 减法：删死虚 + `LeafNode` | 虚函数 50→34；样板归零（17 处） | 无 | 0.5 天 | — |
| **R2** | 字段搬家：4 个部件就位 | `LayoutParams`/`BoxMetrics`/`Interaction`/`VisualState` 成为唯一存储 | 无（行为不变） | 1 天 | R1 |
| **R3** | 收权：几何与树的唯一入口 | 绕过点 11→0；遍历 2→1 | 轻（内部） | 1.5 天 | R2 |
| **R4** | `VisualState` 索引化 | 1,688 B→≤128 B，去堆分配 | 无 | 1 天 | R2 |
| **R5** | 测量缓存统一 | 3 套→1 套（key 可扩展） | 中（内部） | 1 天 | R3 |
| **R6** | 目录重组 + `Semantics` 旁挂 | 依赖规则 D1–D4 可断言；`semantics()` 全员实现 | 轻（+ 可选破坏性） | 2 天 | R3–R5 |
| **R7** | （可选，最后）能力接口 | 基类虚函数 ≤ 30 | **破坏性** | 2 天 | R6 + UI-37 |

合计约 7 天（不含 R7）。R7 与 **UI-37（布局 setter 收敛 17→10）** 合并做，不要拆成两次破坏性改动。

---

## 3. R1 — 减法：删死虚 + `LeafNode`

### 目标

- 基类虚函数 **50 → 34**（删 16 个虚函数，见附 A）。
- **17** 个控件的 `arrange()` 样板归零（见附 B；`CollapsibleHeader`/`TextInput`/`FileBrowser` 不属样板）。
- 不动任何布局/绘制语义。

### 清单

1. `src/ui/widget.hpp`：把附 A 的 16 个虚函数改为普通函数（`clips_children()` 保留为普通函数，因为它是 `overflow()` 的派生）。
2. `src/ui/widget.hpp` / 新文件 `src/ui/node/leaf_node.hpp`：新增
   ```cpp
   class LeafNode : public Widget {
   public:
       void arrange(Rect slot) override { set_geometry(slot); }
       glm::vec2 measure(glm::vec2) override { return clamp_size(preferred_size()); }
   };
   ```
3. 附 B 的 17 个控件改继承 `LeafNode`，删除各自的 `arrange()`；其中 `color_rect` / `image` / `list_view` 的 `measure()` 若只是 `clamp_size(preferred_size())` 也一并删除。
4. `src/ui/core/spacer.hpp`：`measure()` 保留（它返回构造期 `preferred_`，本就是特例）。

### 新增测试

- `test/test_ui_node_contract.cpp`（新文件）：
  - 断言 `LeafNode` 子类的 `arrange` 等价于 `set_geometry`（用探针控件比较 `local()/global()`）。
  - 断言删除的 16 个函数不再是虚函数：**用"派生类同名函数不再被多态调用"的反证**（探针派生一个同名 `set_pos`，经基类指针调用，断言基类版本生效）。
  - 断言 `clips_children()` 与 `overflow()` 的派生关系对全部控件成立（遍历 43 个类型）。

### 验收

```bash
ninja -j4 -C build && cd build && ctest            # 74/74
grep -c "    virtual " src/ui/widget.hpp            # 期望 34
./build/test/test_ui_style_snapshot.exe             # 哈希必须仍是 c8a0966c / eb32c524
```

### 预期 diff

实际：**21 文件、+528/−142**（新增 `leaf_node.hpp` + `test_ui_node_contract.cpp`，17 个控件各减 4~6 行 + 16 处 `virtual` 关键字）。

### 风险 / 回滚

- 风险：某控件其实依赖了 `arrange` 覆写被多态调用（经基类指针 `Widget*` 调 `arrange`）。探测：`grep -rn "arrange(" src | grep "Widget\*\|Widget&"`，若有则回退该控件。
- 回滚：`git revert` 单提交即可，无状态残留。

---

## 4. R2 — 字段搬家：4 个部件就位

### 目标

把 36 个散落字段按语义聚成 4 个按值内嵌的部件，**行为零变化**、**43 个控件的调用点零修改**（现有访问器保留签名，内部转发）。

### 清单

| 新部件 | 位置 | 吞并字段（现在都在 `widget.hpp`） |
|---|---|---|
| `LayoutParams` | `src/ui/layout/layout_params.hpp` | `preferred_size_ min_size_ max_size_ size_percent_ fill_x_ fill_y_ aspect_ratio_ flex_grow_ align_self_ has_align_ absolute_pos_ display_` |
| `BoxMetrics` | `src/ui/layout/box_metrics.hpp` | `padding_ padding_override_ margin_` |
| `Interaction` | `src/ui/input/interaction.hpp` | `hovered_ focused_` + 新增 `enabled pressed dragging selected`（R2 只加字段，不接线） |
| `VisualState` | `src/ui/style/visual_state.hpp` | `transition_`（R2 先原样搬入，R4 再索引化） |
| 保留在 `Widget` | — | `geometry_`(=`local_ global_`) `children_` `parent_` `context_` `alive_token_` `visible_` `z_index_` `tab_order_` `debug_` `layout_dirty_` `notified_` `measure_*` `content_extent_` `radius_override_` |

步骤：

1. 新建 4 个头，把字段**原样**放进去（`StyleTransition` 暂不改）。
2. `widget.hpp` 改成持有部件成员，并把**全部现有访问器**改为转发（`min_size()` → `layout.min_size`，`padding()` → `box.padding`…）。
3. `set_context` / `set_theme` 相关的字段级逻辑不动。
4. 检查直访：`grep -rn "padding_\.\|margin_\.\|min_size_\b" src/ui | grep -v widget.hpp` 应为 0（现状本来就是 0，搬家后仍然是 0）。

### 新增测试

- `test/test_ui_state_ownership.cpp`：
  - 断言 `sizeof(Interaction) <= 8`（5 个 bool + padding）。
  - 断言 `Interaction::state()` 与旧自由函数 `control_state()` 对全部 2×2×2×2 = 16 种组合输出一致（先把旧函数保留为参考实现，R3 再删）。

### 验收

```bash
ctest                                   # 74/74
./build/test/test_ui_style_snapshot.exe # 哈希不变
grep -rn "padding_\.\|margin_\." src/ui | grep -v "widget.hpp" | grep -v "box\." | wc -l   # 0
```

### 预期 diff

1 个新头 ×4 + `widget.hpp` 重排；**43 个控件 0 修改**是硬要求（若必须改控件，说明访问器没对齐，停下来先对齐）。

### 风险 / 回滚

- 风险：`&member`/`std::swap(member, …)` 之类的引用语义被破坏。探测：`grep -rn "&padding_\|&margin_\|&min_size_" src`。
- 回滚：单提交 revert。

---

## 5. R3 — 收权：几何与树的唯一入口

### 目标

- 几何写绕过点 **11 → 0**（附 C）。
- `children_` 私有；遍历收敛到 `for_each_child()`（现状 76 处引用，其中 `splitter` 25、`tab_view` 15、`grid.cpp` 11）。
- `control_state()` 自由函数删除，改 `input.state(checked)`。

### 清单

1. `widget.hpp`：`global_`/`local_` 改名 `geometry_.slot` 并移入 `protected` 的 **private 区**；暴露 `Rect local() const` / `Rect global() const`（读点 255 处**不动**，因为 `append_to` 里到处是 `global_.pos`——R3 **只做写收权**，读收权留到 R6 可选）。
   - 为减小 diff：保留 `global_`/`local_` 作为 `Geometry` 的**引用型访问器**不做；改为**保留成员名但设为 private + 提供 `global_` 同名的 protected 内联只读访问器？** 见下方"决策"。
2. 修 11 处写点（附 C）为 `set_geometry(...)` 或新增 `set_global_bounds()` 的窄化入口（如 `window.hpp:307` 的 `global_.size = {width(), height()}` → `set_geometry({global().pos, {width(), height()}})`）。
3. `children_` 私有；新增
   ```cpp
   std::size_t child_count() const;
   Widget* child_at(std::size_t) const;
   template <class F> void for_each_child(F&&) const;   // children_ + overlays()，语义见 K2
   ```
4. 迁移 `splitter`（25）、`tab_view`（15）、`grid.cpp`（11）、`root.cpp`（5）、`wrap.cpp`（5）、`linear_layout.cpp`（5）等引用到 `for_each_child`。
5. 4 个遍历点统一（`set_context` / `clear_layout_dirty_recursive` / `invalidate_measure_recursive` / `layer_paint_order`）改用 `for_each_child`；`debug_recursive()` 顺带补上 overlays（现状漏掉）。

### 决策（重要，避免 255 处 diff 爆炸）

- **读** `global_`/`local_`：R3 **保留** `protected` 只读访问（`const Rect& global_`），因为 append_to 里有 255 处 `global_.pos`；把它们改成 `global()` 是 R6 的机械替换项，单独一批做。
- **写**：必须走 `set_geometry()`，这条是 K3，R3 内闭合。

### 新增测试

- `test/test_ui_tree_contract.cpp`：
  - `Section`/`PreviewPane` 场景：`for_each_child` 能覆盖真实子项（修复现状"`children()` 与 `children_` 不一致"）。
  - `debug_recursive()` 覆盖 overlays。
  - 断言 `Widget::children()` 不再是虚函数（R3 后它只是 `for_each_child` 的兼容包装或删除）。

### 验收

```bash
ctest                                   # 74/74
./build/test/test_ui_style_snapshot.exe # 哈希不变
grep -rn "global_ = \|global_\.pos = \|global_\.size = " src/ui | wc -l   # 0（`widget.hpp` 内 set_geometry 除外）
```

### 风险 / 回滚

- 风险：`section.hpp` / `preview_pane.hpp`（`src/cosine`）依赖 `children()` 虚覆写。R3 前先确认：`grep -rn "children() const override" src | wc -l` → 现为 2 处 src + 2 处 test；需同时给它们提供新的挂载方式（`for_each_child` 覆写或改用 `adopt`）。**这是 R3 唯一的破坏点**，单独一步、单独验证。

---

## 6. R4 — `VisualState` 索引化（→ ≤128 B、0 堆、可共享）

### 目标

- `sizeof(StyleTransition)` 由 **1,688 → ≤ 128 B**，且**内嵌**（不再 `make_unique`）。
- 内存：交互过的控件由 ≈1,970 B → ≈380 B（−80%）。
- 语义不变：分槽、`from = lerp(from,to,progress)`、revision 逐槽重建、**一帧一角色一状态**。

### 清单

1. `style/theme` 增加**样式索引表**：
   ```cpp
   // Theme 内：12 角色 × 5 状态 = 60 个 BoxStyle，按 (role,state) 索引
   std::uint16_t style_index(StyleRole, ControlState) const;
   BoxStyle      style_at(std::uint16_t index) const;
   ```
   实现：`Theme` 构造时一次性填充 `std::array<BoxStyle, 60>`（可由现有 `Theme::style()` 生成，保证零行为变化）。
2. `StyleSlot` 改为 `{ControlState state; uint16_t from, to; float progress;}`（8 B）。
3. `VisualState`：`std::array<StyleSlot,12> slots` + `uint64_t revision` + `uint16_t started`，内嵌在 `Widget`，删 `unique_ptr<StyleTransition>` 与 `widget_style.hpp` 的旧结构。
4. `widget.cpp::visual_style()` 迁入 `VisualState::style(role, state, theme)`；`Widget::visual_style()` 保留为一行转发（25 处调用点不动）。
5. 删除 `widget_style.hpp` 里的 `StyleSlot{from,to}` 版本；`kStyleRoleCount` 的 `static_assert` 移入 `visual_state.hpp`。

### 新增测试

- `test/test_ui_visual_state.cpp`：
  - `sizeof(VisualState) <= 128`。
  - 无堆分配：`Widget` 构造 + 首次 `visual_style()` 后 `sizeof` 不变（无指针成员）。
  - 语义等价：对 12×5 组合，`VisualState` 输出 == `Theme::style()`（逐字段比较）。
  - 过渡：`transition_ms > 0` 时中间值落在 `[from,to]`；中途反向不跳变（沿用现有 `test_ui_transition` 的断言并迁移一条）。
  - **一帧一角色一状态**：同一控件同帧调用 `style(Track,Normal)` 与 `style(Thumb,Normal)` 结果不同（UI-46 教训固化为断言）。

### 验收

```bash
ctest                                   # 74/74
./build/test/test_ui_style_snapshot.exe # 哈希允许变化一次，需人工确认后更新基准
```

> R4 是**唯一**允许快照哈希变化的批次（因为颜色改为按索引取，浮点路径可能不同）。变化必须显式记录在提交信息里，并附新旧哈希。

### 风险 / 回滚

- 风险：索引表与 `Theme::style()` 不同源 → 颜色漂移。缓解：单测逐字段比较 60 组合。
- 回滚：单提交 revert（索引表是新增的，删除即回到旧路径）。

---

## 7. R5 — 测量缓存统一

### 目标

三套缓存 → 一套（K6）：

| 现状 | 位置 | 处置 |
|---|---|---|
| 基类 `(constraints, layout_epoch)` | `widget.hpp:534-550`，使用者仅 `scroll_panel`、`tab_view` | 保留为 `MeasureCache` 的基础键 |
| `Label::MeasureKey`（文本+字号+换行宽+图集） | `label.hpp:180-200` | 迁入 `MeasureCache::Key` 的**可扩展字段** |
| `TreeView::cached_visible_rows()` | `tree_view.hpp` | 保留为"派生数据"，但统一走 `MeasureCache` 的 invalidation 信号 |

### 清单

1. `layout/measure_cache.hpp`：
   ```cpp
   struct MeasureKey {
       glm::vec2 constraints;
       std::uint64_t epoch;
       // 可选派生输入（默认零值 = 不参与比较）
       std::string_view text;
       float font_size = 0.0f;
       float wrap_width = -1.0f;
       const void* resource = nullptr;   // FontAtlas 等
   };
   ```
2. `Widget::measure_child` 与 `Label::measure` 共用同一 `MeasureCache` 实例与同一 `invalidate` 路径。
3. `TreeView` 的可见行缓存改为订阅同一 invalidation（`invalidate_measure_recursive` 时一并失效）。
4. 删除 `Label` 的 `MeasureKey/MeasureCache` 私有实现（约 20 行）。

### 新增测试

- `test/test_ui_measure_cache.cpp`：
  - 同帧同约束二次 `measure_child` 命中（计数探针）。
  - 改文本 → `Label` 重测；改主题字号 → `Label` 重测（覆盖现状"主题改但缓存未失效"的隐患）。
  - `TreeView` 的行缓存随 `invalidate_measure_recursive` 失效。

### 验收

```bash
ctest
grep -rln "MeasureKey\|cached_visible_rows" src/ui | wc -l    # 期望 1（仅 measure_cache.hpp 提及）
```

---

## 8. R6 — 目录重组 + `Semantics` 旁挂

### 目标

- 目录按 §3 语义分层（`geom/ layout/ tree/ input/ style/ node/ semantics/ widgets/ containers/ composites/ system/`）。
- 依赖规则 D1–D4 变成**可断言的测试**。
- `semantics()` 在 43 个控件上实现（多数是 3 行）。
- 状态字段全部收敛到 `Interaction`（`enabled_` 8 → 0、`pressed_` 3 → 0、`dragging_` 5 → 0、`selected_` 6 → 保留索引 + 投影）。
- 可选：`global_`/`local_` 的 255 处读点机械替换为 `global()`/`local()`。

### 清单

1. 建目录 + `git mv`（纯移动，单独一步，便于 review）。
2. 加 `test/test_ui_architecture.cpp`：用 `grep` 的等价断言检查 D1–D4（实现为 C++ 里读源码路径做字符串断言，或作为 CTest 的 `grep` 步骤）。
3. `Semantics`（见设计 §4.5）：43 个控件实现 `semantics()`；`enabled/selected/expanded` 从 `Interaction` 派生。
4. 8 个控件的 `bool enabled_` 改为 `input.enabled`；`set_enabled()/is_enabled()` 提到 `Widget` 层。
5. `pressed_`（3）/`dragging_`（5）改 `input.pressed` / `input.dragging`。
6. 删除 `style.hpp:50` 的 `control_state()` 自由函数，全部改 `input.state(checked)`。
7. `widget.hpp` 拆分：`node/node.hpp`、`node/layout_node.hpp`、`node/leaf_node.hpp`、`node/container_node.hpp`。

### 新增测试

- `test/test_ui_semantics.cpp`：遍历 43 个控件实例，断言
  - 每个可聚焦控件 `semantics().role != Custom`；
  - 每个容器 `is_container()`（或 `for_each_child` 非空时）语义 role ∈ {Panel, ScrollView, Dialog, Menu, List, Table, Tree}；
  - `set_enabled(false)` 后 `semantics().enabled == false` 且 `input.state() == Disabled`（对 8 个控件逐一断言）。
- `test/test_ui_architecture.cpp`：D1–D4 四条依赖规则。

### 验收

```bash
ctest                                                    # 74 + 新增
grep -rc "bool enabled_" src/ui                          # 0
grep -rn "control_state(" src/ui | wc -l                 # 0
wc -l src/ui/node/*.hpp | sort -n | tail -1              # ≤ 250
./build/test/test_ui_style_snapshot.exe                  # 哈希再次变化需显式记录（尺寸/改名无关，样式不变则不变）
```

### 风险 / 回滚

- 风险：`git mv` + 大规模 include 重写导致冲突；缓解：目录移动单独一步、单独提交，其它改动在移动之后做。
- 风险：`Semantics` 变成第二套真值；缓解：K4 契约——`semantics()` 必须是 `const` 派生，禁止 setter（代码评审 + 单测覆盖 16 种状态组合）。

---

## 9. R7（可选，破坏性）能力接口

**只在 R1–R6 全绿之后做，且与 UI-37 合并。**

- 目标：基类虚函数 ≤ 30；交互能力从基类平铺改为按能力分组（`PointerTarget` / `KeyTarget` / `FocusTarget`）。
- 代价：43 个控件 + `UISystem` 的 dispatch 需要按能力查找；用 `traits` 位图替代 `dynamic_cast`。
- **不建议**在没有 R1–R6 的收权前提下先做 R7（会退化成"6 个基类"的老问题）。

---

## 10. 依赖关系与并行度

```
R1 ──► R2 ──► R3 ──► R5 ──► R6 ──► R7
        └────► R4 ────┘
```

- R4 只依赖 R2，可与 R3 **并行**（不同文件：R3 动几何/树，R4 动样式）。
- R5 必须在 R3 之后（要复用 `for_each_child` 做递归失效）。
- R6 最后（目录移动会让所有 diff 变噪音）。
- R7 独立成批，且需设计评审。

---

## 11. 全局 DoD（每批都要过）

1. `ninja -j4 -C build` 无新警告（`-Wall -Wextra` 下）。
2. `ctest` 全绿（当前 74，逐批递增）。
3. 快照哈希：R1–R3 必须 `c8a0966c` / `eb32c524`；R4/R6 若变化须在提交信息中注明新旧值。
4. `sele check` 通过。
5. 指标看板（§1）对应行更新。
6. 三入口冒烟：`cosine` / `ui_lab` / `ui_gallery` 无 `VUID-` / `validation` 输出。
7. 本批不引入新的 `grep` 违规（K1–K6 的探测命令全部为 0 或达标）。

---

## 12. 风险登记

| # | 风险 | 影响 | 缓解 |
|---|---|---|---|
| T1 | `Section`/`PreviewPane` 依赖 `children()` 虚覆写 | R3 卡住 | R3 单独一步：先提供 `for_each_child` 覆写路径，再删虚访问器 |
| T2 | `global_` 255 处读点 | 若 R3 一起改，diff 失控 | R3 只收"写"；读的机械替换放 R6 |
| T3 | 颜色改索引导致像素漂移 | R4 快照变化 | 60 组合逐字段等值断言；快照变化需人工确认 |
| T4 | `git mv` 与本批逻辑改动混在一起 | review 困难 | 目录移动单独提交 |
| T5 | `Semantics` 成为第二套真值 | 长期新坑 | K4 + 只允许 `const` 派生 |
| T6 | `VisualState` 内嵌后 `Widget` 变大 | 内存不降 | R4 前后各测一次 `sizeof`，以"交互过控件的总内存"为指标（1,970 → 380） |
| T7 | 42 个控件一次性迁移导致构建长期红 | 阻塞其它工作 | 每批 ≤ 1 天、随时可 revert；不做长驻分支 |

---

## 13. 不在本计划范围

- `src/anim`（文档模型改名/codec 拆分）、`src/cosine`（编辑器拆分）、`src/backend`：见另一份计划。
- `src/ui/system/`（`UISystem` 558 行）的拆分：R1–R6 期间不动，只把状态写入点改为 `input.*`。
- 无障碍平台接入：本轮只做 `Semantics` 结构，不接平台。
- 约束求解 / 立即模式：明确否决（见设计 §10）。

---

## 附 A：16 个从未被覆写的虚函数（R1 删除清单）

> 校验方式：`src` + `examples` + `test` 全仓 `override` 命中数 = **0**。

| # | 函数 | 说明 |
|---|---|---|
| 1–2 | `set_pos(float,float)` / `set_pos(vec2)` | 改为普通函数 |
| 3–4 | `set_size(float,float)` / `set_size(vec2)` | 改为普通函数 |
| 5 | `set_global_bounds(Rect)` | 改为普通函数（R3 起并入 `set_geometry`） |
| 6 | `parent()` | 改为普通函数 |
| 7 | `clips_children()` | 改为普通函数（由 `overflow()` 派生） |
| 8 | `debug()` | 改为普通函数 |
| 9–12 | `padding_left/right/top/bottom()` | 改为普通函数（或直接删除，统一读 `box.padding`） |
| 13–16 | `margin_left/right/top/bottom()` | 同上 |

> 计数口径：**16 个虚函数 / 14 个名字**（`set_pos`、`set_size` 各 2 个重载）。删后虚函数 50 → **34**。

## 附 B：纯样板 `arrange()`（R1 下沉清单）——**实测 17 个，已清零**

```
button checkbox color_input color_rect drag_float image label preview_ruler
progress radio slider spacer tab_bar tooltip list_view scene_view timeline_view
```

**例外（3 个，不属样板，R1 未迁移）**：

| 控件 | arrange 除 `set_geometry` 还做了什么 |
|---|---|
| `collapsible_header` | 额外写 `global_ = {slot.pos, {slot.size.x, height()}}`（高度由标题决定） |
| `text_input` | 额外调 `clamp_scroll()` |
| `file_browser` | 额外把几何转给内部 `table_` |

> 校正：初版计划估「20 个」，实测只有 17 个真正只有一行；上表 3 个被误计入。

判定命令（期望输出 0）：

```bash
for f in $(grep -rl "void arrange(Rect slot) override" src/ui --include=*.hpp); do
  body=$(awk '/void arrange\(Rect slot\) override/,/^    }/' $f | grep -v "void arrange\|^    {\|^    }\|^\s*$" | sed 's/^ *//')
  [ "$body" = "set_geometry(slot);" ] && echo "残留: $(basename $f)"
done | wc -l
```

## 附 C：11 处几何写绕过点（R3 修复清单）

| 文件:行 | 现状 | 改法 |
|---|---|---|
| `composites/menu_bar.hpp:129` | `global_ = slot;` | `set_geometry(slot);` |
| `composites/menu_bar.hpp:150` | `global_ = local_;` | `set_geometry(local());` |
| `composites/window.hpp:265` | `global_ = {{0,0}, area};` | `set_geometry(...)` |
| `composites/window.hpp:270` | `global_ = {restore_pos_, restore_size_};` | `set_geometry(...)` |
| `composites/window.hpp:307` | `global_.size = {width(), height()};` | `set_geometry({global().pos, {width(), height()}})` |
| `composites/window.hpp:525` | `global_.pos = next;` | `set_geometry({next, global().size})` |
| `composites/window.hpp:560` | `global_.pos = next_pos;` | 同上 |
| `composites/window.hpp:562` | `global_.size = next_size;` | 同上 |
| `containers/scroll_view.cpp:175` | `global_ = slot;` | `set_geometry(slot);` |
| `core/collapsible_header.hpp:65` | `global_ = {slot.pos, {slot.size.x, height()}};` | `set_geometry(...)` |
| `core/scrim.hpp:43` | `global_ = slot;` | `set_geometry(slot);` |

（`widget.hpp:80/88/95` 是 `set_global_bounds`/`set_geometry`/`set_arranged_pos` 自身，属于合法入口。）

## 附 D：状态字段清单（R6 收敛）

| 字段 | 处数 | 位置 |
|---|---|---|
| `bool enabled_` | **8** | `dropdown:299` `button:120` `checkbox:127` `color_input:237` `drag_float:202` `radio:123` `slider:206` `text_input:547` |
| `bool pressed_` | **3** | `dropdown:300` `button:121` `checkbox:128` |
| `bool dragging_` | **5** | `drag_float:203` `slider:207` `splitter:279` `text_input:548` `timeline_view:540` |
| `int selected_` | **6** | `dropdown:297` `radio_group:121` `tab_bar:265` `file_browser:412` `list_view:205` + `bool selected_`@`radio:122` |
| 其他选中态 | **1** | `timeline_view:537 selected_track_` |

## 附 E：三套测量缓存（R5 收敛）

| 缓存 | 位置 | 使用者 |
|---|---|---|
| 基类（constraints + epoch） | `widget.hpp:534-550` | `scroll_panel`、`tab_view` |
| `Label::MeasureKey` + `MeasureCache` | `label.hpp:180-200` | `Label` |
| `cached_visible_rows()` | `tree_view.hpp` | `TreeView` |

## 附 F：样式与焦点环的分散点（R4/R6）

| 现象 | 处数 |
|---|---|
| `visual_style(` 调用 | **25**（`widget.cpp` 外） |
| `append_focus_ring(` 调用 | **13** |

## 附 G：对 `docs/review/ui-widget-v1-*.md` 的计数更正

> 旧文档不改；此处记录更正，避免后续按错的数字做计划。

| 旧文档的说法 | 实际（实测） |
|---|---|
| "3 处直接 `children_.push_back` 绕过 `adopt`" | **0 处**。`grep` 的 3 个命中是：`widget.hpp:110` 的一句注释、`widget.hpp:115` 的 `adopt()` 内部、`widget.hpp:130` 的 `raise_child()` 内部。`PreviewPane` 的同名成员在 `src/cosine`，是本轮范围外 |
| "`selected_` 4 处" | **6 个 `selected_` + 1 个 `selected_track_`**（见附 D） |
| "删 14 个死虚 → 虚表 50→36" | 删的是 **16 个虚函数**（14 个名字，`set_pos`/`set_size` 各 2 重载）→ **50→34** |

## 附 H：R1 结束后的自检脚本（可粘贴执行）

```bash
# 1) 虚函数数
grep -c "    virtual " src/ui/widget.hpp                      # 期望 34
# 2) 样板 arrange 归零
for f in $(grep -rl "void arrange(Rect slot) override" src/ui --include=*.hpp); do
  n=$(awk '/void arrange\(Rect slot\) override/,/^    }/' $f | grep -c "set_geometry(slot);")
  l=$(awk '/void arrange\(Rect slot\) override/,/^    }/' $f | wc -l)
  [ "$n" = "1" ] && [ "$l" -le 5 ] && echo "残留: $f"
done
# 3) 测试与快照
cd build && ctest && ./test/test_ui_style_snapshot.exe
```

---

## 附 I：进度记录

| 批 | 状态 | 完成时间 | 实测结果 | 与计划的偏差 |
|---|---|---|---|---|
| R1 (T42) | **done** | 2026-11-27 | 虚函数 50→34；样板 arrange 17→0；`test_ui_node_contract` 22 条断言；ctest **75/75**；快照 `c8a0966c` / `eb32c524` 不变；三入口冒烟 0 validation | 样板数 20→**17**（`collapsible_header`/`text_input`/`file_browser` 的 arrange 另含语义）；实际 21 文件 +528/−142 |
| R2 (T43) | **done** | 2026-11-27 | 4 部件按值内嵌（LayoutParams 72B / BoxMetrics 36B / Interaction 6B / VisualState 8B）；`sizeof(Widget)` 280→288；`test_ui_state_ownership` 20 条断言（16 组合等价）；ctest **76/76**；快照 `c8a0966c`/`eb32c524` 不变 | 字段名 `layout` 与既有虚函数 `layout()` 冲突 → 改用 `layout_`（尾下划线）；控件目录 **0 文件改动**（达成） |
| R3 (T44) | todo | — | — | — |
| R4 (T45) | todo | — | — | — |
| R5 (T46) | todo | — | — | — |
| R6 (T47) | todo | — | — | — |
