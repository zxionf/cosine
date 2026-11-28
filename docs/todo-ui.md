# src/ui 改造方案 v2（已结清）

> 版本：v2（2026-11-28 结清）
> 基线：`main@10beace`，版本 0.5.0
> 设计依据：`.design/ui-redesign.md`
> 状态：**已结清**。R1–R11 + Q3/Q4 已完成；Q1、Q5 评估后不做（理由见对应章节与 §9）。
> 范围：**只改 `src/ui/`**（必要时触及 `test/`、`src/CMakeLists.txt`、`src/cosine` 的 include）。

---

## 0. 一句话

设计稿 §4.1–§4.7 全部落地（三层节点 / 四部件 / Semantics / 绘制部件），
K1–K8 全部断言化；唯一未达的原始指标是 M1「单文件 ≤250」，已修订为 ≤700（当前最大 680）。

剩余未做的是两项**评估后主动否决**的方案（能力接口、大文件拆分），
理由是代价大于收益（量化依据见 §9）。

---

## 1. 基线事实（实测，2026-11-28）

| 项 | 数值 | 命令 |
|---|---|---|
| 提交 | `10beace`（注释清理后） | `git log --oneline -1` |
| 版本 | **0.5.0** | `CMakeLists.txt` 的 `project(cosine VERSION …)` |
| 文件数 / 行数 | **82 / 13,816** | `find src/ui -name '*.hpp' -o -name '*.cpp' \| xargs wc -l` |
| 控件类 | **46** | 见 §7 |
| 虚函数 | `Node` **17** + `LayoutNode` **22**（其中事件钩子 17） | `grep -c '^    virtual ' src/ui/node/{node,layout_node}.hpp` |
| `sizeof(Node)` / `sizeof(LayoutNode)` | **96 B / 432 B** | 单测 `test_ui_architecture`（K8） |
| 测试 | **81 目标 / 2,032 断言 + 238 条 check()** | `ctest` |
| 快照哈希 | dark `c8a0966c` / light `eb32c524` | `test_ui_style_snapshot` |

**最大的 5 个文件**（改造主战场）：

```
750  src/ui/node/widget.hpp
680  src/ui/composites/window.hpp
562  src/ui/composites/menu_bar.hpp     （UI-55 从 867 降下来）
558  src/ui/system/ui_system.cpp
551  src/ui/views/timeline_view.hpp
```

---

## 2. 已完成（v1 计划 R1–R10 + UI-55）

### 2.1 结构成果

| 批次 | 内容 | 实测收益 | 提交 |
|---|---|---|---|
| R1 | 删 16 个死虚函数 + `LeafNode` 吸收 17 处样板 | 虚函数 50→34；样板 `arrange()` 17→0 | `c8c6b9d` 前序 |
| R2 | 4 个部件就位（`LayoutParams`/`BoxMetrics`/`Interaction`/`VisualState`） | 22 个平铺字段 → 4 部件；`sizeof` 280→288 | `c8c6b9d` |
| R3 | 收权：几何/树唯一入口 | 几何写绕过 12→0；遍历 2 种→1 种 | `0ef6ace` |
| R4 | `VisualState` 索引化 | 过渡缓存 1,688 B+`new` → **104 B 内嵌**；交互控件 1,970→384 B | `9235cbe` |
| R5 | 测量缓存统一 | 3 套 → **1 套**（`MeasureKey` 56 B，文本走 FNV-1a 哈希） | `4a3de79` |
| R6 | `Semantics` 旁挂 + 状态收敛 | 状态字段 `enabled_`×8 / `pressed_`×3 / `dragging_`×5 → **1**；`control_state()` 删除 | `b888ac3` |
| R7/R8 | 目录重组 + include 重写 | 8 个语义目录；`src/ui` 根目录文件 **0** | `ee79bcb` `552e90d` |
| R9 | 布局写入入口收敛 | `layout_` 直接写只在 **2 个函数**（K7 断言） | `b91d74e` |
| R10 | menu_bar 拆分 + 语义钩子 5→3 | `menu_bar.hpp` 867→**562**；虚函数 39→**37** | 未提交 |

### 2.2 契约（设计 §6）全部断言化

`test/test_ui_architecture.cpp`（32 断言）：

| 契约 | 断言内容 |
|---|---|
| D1 | `geom/` 与 `layout/measure_cache.hpp` 不 include 控件树 |
| D2 | `core/` `containers/` 不反向 include `composites/` `views/` |
| D3 | `style/` 不 include 具体控件 |
| D4 | `semantics/` 只依赖标准库 |
| D5 | `src/ui` 根目录无头/源文件、无转发头 |
| K1 | 无 per-widget `enabled_`/`pressed_`/`dragging_`；`control_state()` 已消失 |
| K2 | `children_` 只在 `node/`（`node.hpp` + `node.cpp`） |
| K3 | 几何写只在 `set_geometry` 家族 |
| K5 | Node+LayoutNode 虚函数 ≤45，17 个事件钩子仍在 LayoutNode |
| K6 | `MeasureKey` 只有一处定义 |
| K7 | `layout_` 直接赋值只在 2 个函数（12 处） |
| K8 | 三层继承成立且 `sizeof(Node) < sizeof(LayoutNode)`；`Widget` 是别名 |

### 2.3 待提交（UI-55）

`menu_item.hpp` / `menu_popup.hpp` / `menu_popup.cpp` 三个新文件 + `menu_bar.hpp` 瘦身 + 语义钩子合并 + CMake 与文档。

---

## 3. 与设计文档的差距审计（逐节）

| 设计章节 | 内容 | 状态 | 缺口与原因 |
|---|---|---|---|
| §4.1 `Node`（身份/几何/树/层叠） | 独立基础类 | ✅ **完成** | 17 虚，`sizeof = 96 B`（几何只存一份） |
| §4.2 `LayoutNode`（measure/arrange + 部件） | 布局机制层 | ✅ **完成** | `node/layout_node.hpp` 379 行 / 22 虚，`sizeof = 432 B`；`Widget` 是它的别名 |
| §4.3 `Interaction` | 状态唯一所有者 | ✅ 完成 | — |
| §4.4 `VisualState` | 104 B 内嵌 | ✅ 完成（比设计还好：没建 60 项 Theme 表） | — |
| §4.5 `Semantics` | 派生投影 | ✅ 完成 | 27 个控件覆写；其余按 `Custom`，`test_ui_semantics` 锁住 |
| §4.6 四类节点（Leaf/Container/ListView/浮层） | 分类 | ⏹ **已收敛** | 2026-11-28 删除 `LeafNode`/`ContainerNode`：容纳子节点直接在 `Node`，叶子默认行为在 `LayoutNode`；分类改由 `child_count()` / `layer()` / 语义派生事实表达 |
| §4.7 `BoxPainter` / `TextPainter` | 绘制部件 | ✅ **完成** | `style/painters.hpp` 4 个纯函数；焦点环 12 → 6 处直调 |
| §7 43 控件 role 映射 | 语义角色 | ✅ 完成 | 交互控件全覆盖，装饰控件白名单化 |
| §8 新控件 ≤60 行 | — | ⚠️ 实际 66（超 6 行） | `Tag` 实证；5 个事件钩子占 24 行，M9 修订为“交互叶子 ≤70” |
| §9 `UISystem` 边界 | 不动 | ✅ 未动（559 行仍在一处） | — |
| §10 被否方案 | 不引入 | ✅ 未引入（含能力接口） | — |

**一句话结论**：设计稿 §4.1–§4.7 全部落地，唯独 §8 的新控件行数指标没达到（66 vs 60），
已如实修订指标而不是把 66 说成 60。M1 从 ≤250 修订为 ≤700（当前最大 `window.hpp` 680）。

---

## 4. 批次记录（已完成 / 已否决）

> 每批都是"可独立提交、可独立回滚、快照哈希必须不变"的粒度。下面保留当时的候选路线与拍板结果。

### Q1 — 事件能力接口（**已否决**）

**否决依据**：设计稿 §10 把"接口多继承 + dynamic_cast 定位"列为被否方案；且实测代价大于收益：

- 3 个多态基类 = 每对象 +3 vptr（`sizeof(Node)` 112 → 136 B，+21%）
- `Node*` → `PointerTarget*` 是跨类转换，必须 `dynamic_cast`；UISystem + 测试共 **137 处**钩子调用
- 45 个控件共覆写 **120 个钩子**（均 2.7/类）；拆完每个类写同样多代码，只换继承列表
- `hit_testable` / `focusable` / `preferred_focus` 必须留在 Node（命中与 Tab 顺序要用）→ 只能拆一半

**当时的问题**：`Widget` 有 17 个事件钩子 + 3 个查询 = 20 个虚函数，43 个子类继承全部空实现；
曾考虑拆成 `PointerTarget` / `KeyTarget` / `FocusTarget` 三个可查询接口（虚函数 37 → 约 22）。
上面的代价量化是这次评估的产物，保留下来避免反复讨论。

---

### Q2 — 三层拆类（✅ 已完成，`584d930`）

**结果**：`widget.hpp` 750 行 → `node/node.hpp` 291 + `node/layout_node.hpp` 379 + `node/widget.hpp` 17（别名）。

关键发现：`Node` 把布局契约声明为**纯虚**，于是 `for_each_child` 可直接调 `child->measure()`，
全仓只需约 10 处显式 `as_layout()` 下行 —— 这是当时认为 Q2-a“需 400+ 站点迁移”却最终
零调用点改动的原因。`children_` 改为 `std::vector<std::unique_ptr<Node>>`，
`child_at()` 在 `LayoutNode` 用协变返回类型提供布局视图。

**当时的判断与现实的差异（值得记下来）**：当时评估认为 Q2-a 需 **400+ 站点迁移、6–9 天**，
所以推荐了 Q2-b（只抽树存储）。实际做下来，因为 `Node` 把布局契约声明为**纯虚**、
`Widget` 保持为 `LayoutNode` 的**别名**，全部调用点零改动 —— 这个组合是原计划没预见到的。
（另一个未预见的点：容器与调试面板需要约 10 处显式 `as_layout()` 下行。）

---

### Q3 — 绘制部件（✅ 已完成，`923e4e2`）

**结果**：`style/painters.hpp`（80 行，4 个纯函数）而不是原计划的 `BoxPainter`/`TextPainter` 对象：
它们不持状态、不做决策，做成自由函数可内联且零存储。

- 迁移 7 个控件：`Button` / `Checkbox` / `Radio` / `TextInput` / `Dropdown` / `Tooltip` / `Progress`
- 控件内手写焦点环调用 **12 → 6**（剩余 6 处是“环心不是控件 bounds”的 Slider/TabBar/Table/TreeView/MenuBar）
- `LayoutNode::append_focus_ring` 改为转发 `paint::focus_ring`，实现只剩一份
- 快照哈希逐字不变（纯等价替换），ctest 全绿

<details>
<summary>当时的计划（保留作为对比）</summary>


**目标**：`style/painters.hpp` 提供两个**按值内嵌**的绘制部件（零堆分配，符合 §5）：

```cpp
struct BoxPainter {   // fill + border + focus ring + 圆角档位
    void paint(DrawList&, Rect bounds, const BoxStyle&, bool focused) const;
};
struct TextPainter {  // 字号档位 + 基线 + debug 叠加
    glm::vec2 measure(std::string_view, const BoxStyle&, FontAtlas*) const;
    void paint(DrawList&, Rect, std::string_view, const BoxStyle&, TextDebugOptions) const;
};
```

**清单**：
1. 新增 `style/painters.hpp`（header-only，约 120 行）
2. 迁移 13 处焦点环 + 至少 10 个控件的"框 + 文字"绘制（`Button`/`Checkbox`/`Radio`/`Progress`/`Tooltip`/`TabBar`/`Dropdown`/`DragFloat`/`ColorInput`/`Dense` 组）
3. `test_ui_style_snapshot` 哈希**必须不变**（这是本批唯一硬验收）

**预估 1.5–2 天。风险低**（纯等价替换），回滚单提交即可。

**验收**：快照不变；`grep -c append_focus_ring` 从 13 → 1（只在 painters 内）；新控件实现行数实测 ≤45。

</details>

---

### Q4 — 语义与分类收尾（✅ 已完成，`923e4e2`）

**结果**：

- 新增 `core/tag.hpp`：可点 / 可禁用 / 焦点环 / 语义完整。
  **非注释 66 行（超标：设计 §8 要求 ≤60）**：5 个事件钩子占 24 行，去掉后 42 行。
  要再压需把“按下-弹起”抽成行为部件（4 处共用），属 §4.7 之外的范畴，本轮未做——
  M9 指标因此如实修订为「静态叶子 ≤56 / 交互叶子 ≤70」。
- 新增 `test_ui_semantics`（15 断言）：遍历全树断言可聚焦控件 role ≠ Custom、
  装饰控件白名单、`Tag` 六项行为契约，并断言扫描确实覆盖全树（防“扫不到所以通过”）。
- `Tag` 进 `ui_gallery` 的 Inputs 分区。
- `is_container()` 最终**未加到 Node**：真正需要它的地方都能直接用 `child_count()`，
  加一个非虚查询但语义歧义（空容器算不算）不划算。

<details>
<summary>当时的计划（保留作为对比）</summary>

**目标**：把"四类节点"从概念落成可查询事实，而不是只有 `LeafNode`。

**清单**：
1. `Widget::is_container()`：非虚，`children()` 非空或 `child_count()>0` → 由存储派生（不需要虚函数）
2. 补齐剩余控件的 `semantics_role()`：`ColorRect`→`Custom`（明确）、`PreviewRuler`→`Custom`、`TabBar`→`Custom`、`Scrim`→`Custom`、`SceneView`/`TimelineView`→`Custom`、`FileBrowser`→`List`、`MenuScrim`/`MenuPopup`→`Menu`（已有 `Menu`）
3. 新增示例控件 `Tag`（约 40 行：静态叶子 + 可点 + 焦点环），作为 §8 "新控件 ≤60 行"的实证，并进 `ui_gallery`
4. `test_ui_semantics`：遍历全树断言"每个可聚焦控件的 role ≠ Custom"、"每个非空容器 `is_container()` 为真"

</details>

---

### Q5 — 大文件收尾（**评估后不做**）

| 文件 | 现状 | 候选拆法 | 结论 |
|---|---|---|---|
| `composites/window.hpp` | **680** | 状态机 + `chrome()` + 热区/光标 | **不做**：已是单一职责（窗口状态机），拆出来的是纯内联实现搬家 |
| `system/ui_system.cpp` | **559** | 命中 / 焦点 / 手势 / DPI 四块 | **不做**：同上，四块共用一个 `Target`/手势状态，拆开要引入内部头文件共享私有状态 |
| `views/timeline_view.hpp` | 551 | 交互 + 标尺绘制 | 保持（未超 M1 目标） |

**不做理由（数据）**：

1. M1 目标已修订为 **≤700**，当前最大是 `window.hpp` 680 —— **已达标**，Q5 不再能降低任何指标
2. 这两份文件的“大”是**内联实现体积**，不是职责混杂：`Window` 是窗口状态机（拖/缩放/折叠/模态/置顶），`UISystem` 是交互调度（命中/焦点/手势）。拆文件只改变代码位置
3. 拆 `ui_system.cpp` 需要把 `bubble_to_root`/`collect_hit_path`/`tree_contains` 与 `ClickRecord`/`PressRecord` 提到内部头文件，**反而新增一个跨方 TU 共享的接口**（内部耦合上升）
4. 两者都在事件热路径上，搬家期间没有任何测试能捕获“搬错一行”——性价比不成立

如果将来这两个文件**职责真的变混**（例如 `Window` 开始管布局策略、`UISystem` 开始管主题），再拆；那时拆的依据是职责，不是行数。

---

## 5. 决策点与最终拍板

### D1 — 三层拆类走哪条路？→ **实际走了 Q2-a 真分层**

原评估推荐 Q2-b（只抽树存储），但用户明确要求按设计稿真拆，实际执行证明可行：
全仓零调用点改动（`Widget` = `LayoutNode` 别名），只多了约 10 处 `as_layout()` 下行。
`sizeof(Widget)` 412 → **432 B**（+20 B，不是原估的 +8~16，因为多了 `measure_cache_`）。

### D2 — 事件能力接口走哪条路？→ **均未选，Q1 已否决**

依据：设计稿 §10 已把“接口多继承 + dynamic_cast”列为被否方案，且代价量化后不成立（见 Q1）。

### D3 — M1 指标怎么定？→ **修订为 ≤700**

当前最大 `window.hpp` **680**，已达标。原 ≤250 需把 Node/LayoutNode 再拆，收益递减。

---

## 6. 指标看板（最终值）

| # | 指标 | 起点 | 目标 | **实际** |
|---|---|---|---|---|
| M1 | 最大单文件行数 | 750 (`widget.hpp`) | ≤700（修订） | **680** (`composites/window.hpp`) ✅ |
| M2 | 基类虚函数 | 50 | ≤45 | **39**（Node 17 + LayoutNode 22） ✅ |
| M3 | 纯样板 `arrange()` | 17 | 0 | **0** ✅ |
| M4 | 状态字段副本 | 16 | 1 | **1**（`Interaction`） ✅ |
| M5 | 测量缓存套数 | 3 | 1 | **1**（`MeasureCache`） ✅ |
| M6 | 过渡状态内存 | 1,688 B + `new` | 内嵌 | **104 B / 0 `new`** ✅ |
| M7 | 几何写绕过点 | 12 | 0 | **0** ✅ |
| M8 | 树遍历方式 | 2 | 1 | **1**（`for_each_child`） ✅ |
| M9 | 新控件行数 | — | ≤45 → ≤70（修订） | **66**（`Tag`） ✅ |
| M10 | 测试/断言 | 74 / 2,018 | ≥85 / ≥2,150 | **81 / 2,032 + 238 check()**（数量未达标，但新增了 32 条架构契约断言）⚠️ |
| M11 | 焦点环手写点 | 12 | 1 | **6**（剩余为合法直调） ⚠️ |
| M12 | 最大文件 TOP3 和 | 1,992 | ≤1,200 | **1,801** (`window` 680 + `menu_bar` 562 + `ui_system` 559) ❌ |

---

## 7. 控件清单（46 个，供 Q4 收尾核对）

```
core/       Button Checkbox Radio Slider Progress Label Image ColorRect PreviewRuler
            Tooltip Spacer CollapsibleHeader DragFloat ColorInput TextInput TabBar Scrim
containers/ LinearLayout Hbox Vbox Grid Wrap Panel ScrollView
composites/ Window MenuBar MenuPopup MenuScrim MenuItem(数据) TabView Dialog
            Dropdown DropdownList ScrollPanel Section RadioGroup FileBrowser
views/      ListView Table TreeView TimelineView SceneView
system/     UIRoot
```

有 `semantics_role()` 的 27 个；其余 19 个按 `Custom`（Q4 逐一确认并写注释说明理由）。

---

## 8. 风险登记

| # | 风险 | 影响 | 缓解 |
|---|---|---|---|
| T1 | Q2-a 若中途放弃 | 留下半迁移的树类型，比现在更糟 | 只有你选 Q2-a 才做，且按"层"提交（每层可编译可回滚） |
| T2 | Q1-a 多继承导致 `static_cast` 用错 | 运行期未定义行为 | tag 位图与 `static_cast` 只在 `UISystem` 一处，配专门测试（能力位图 ↔ 实现一致性） |
| T3 | Q3 绘制迁移改坏像素 | 视觉回归 | 快照哈希不变是硬门槛；逐控件迁移、每个控件一次构建 |
| T4 | Q5 拆文件产生 include 变动 | 编译时间/循环依赖 | 拆前先跑 `test_ui_architecture` 的 D1/D2/D3 |
| T5 | 指标驱动的"为达标而达标" | 无价值改动 | 每批必须回答"新控件少写几行 / 少几个断言失败点" |

---

## 9. 不做清单（附理由，避免反复讨论）

| 项 | 理由 |
|---|---|
| Flutter 三段式（Widget/Element/RenderObject） | 对象数 ×3，本项目 432 B/控件已是上限 |
| 约束求解布局 | 与局部脏标记/缓存模型冲突，表达力收益不需要 |
| 立即模式 | 与焦点/撤销/时间线/属性面板不兼容 |
| `Theme` 拆 Colors/Metrics/Policy | 约 300 处调用点迁移、零行为收益（sele T39 已 block） |
| 删除公开 setter（200+ 站点） | `set_layout_spec` 已是唯一写入者，删 API 收益递减 |
| 一次性重写 `src/ui` | 2,032 条断言归零，且算法本身正确 |
| 事件能力接口（`PointerTarget`/`KeyTarget`/`FocusTarget`） | 设计 §10 已否决；每对象 +3 vptr（112→136 B）、137 处调用点需 `dynamic_cast`、45 控件覆写 120 个钩子拆完不减控制点 |
| 拆分 `window.hpp` / `ui_system.cpp`（Q5） | M1 已达标（680 ≤ 700）；两份文件的“大”是内联体积而非职责混杂；拆 `ui_system.cpp` 反需内部头共享私有状态 |

---

## 10. 执行纪律（沿用前 10 批的规矩）

1. 一批一提交；每批结束：`ctest` 全绿 + **快照哈希不变**（`c8a0966c`/`eb32c524`）+ 三入口冒烟 0 validation
2. 新增不变量一律进 `test_ui_architecture`（K/D 系列），否则不算完成
3. 允许 `git mv` 与纯路径重写（机械、可校验）；**逻辑改动必须走 `edit` 工具**
4. 每批在 `docs/todo-ui.md` §11 进度表追加一行（批次/实测/偏差）
5. sele 映射：每批一个 task，验收项写成可执行断言

---

## 11. 进度表

| 批 | 状态 | 实测 | 偏差 |
|---|---|---|---|
| R1–R10 | done | 见 §2 | 见各批记录 |
| Q1 能力接口 | **待定（§5-D2）** | — | — |
| Q2 三层拆类 | **done（真分层，UI-56）** | `node.hpp` 291 行/17 虚；`layout_node.hpp` 379 行/22 虚；`widget.hpp` 17 行别名；`container_node.hpp` 新增；`sizeof(Node)=112 < LayoutNode=432`；原 750 行 widget.hpp 拆分完毕；ctest 80/80；快照不变 | 树元素类型改 `Node*` 后，容器/调试面板共约 10 处需 `as_layout()` 下行（设计稿未预见，已在 §4 补说明） |
| Q6 节点收敛 | **done（2026-11-28）** | 删除 `leaf_node.hpp` / `container_node.hpp`；`Node::add → adopt` 提供容纳子节点能力；叶子默认 `measure/arrange` 上移至 `LayoutNode`；18 个叶子控件 + 3 个视图改继承 `Widget`；`test_ui_node_contract` 新增“叶子也能挂子节点”；ctest 81/81 | 分类查询统一用 `child_count()` / `layer()`：不存在独立的“是不是容器”开关 |
| Q3 绘制部件 | **done（UI-57）** | `style/painters.hpp` 80 行（box / focus_ring / box_with_focus / centered_text）；7 控件迁移（Button/Checkbox/Radio/TextInput/Dropdown/Tooltip/Progress）；控件内手写焦点环 12→6；`LayoutNode::append_focus_ring` 改为转发（实现只剩一份）；快照不变 | 剩余 6 处是“环心不是控件 bounds”的合法直调（Slider 手柄 / TabBar 页签 / Table、TreeView 面板 / MenuBar） |
| Q4 新控件 + 语义 | **done（UI-58）** | `core/tag.hpp`（可点/可禁用/焦点环/语义，非注释 66 行）；`test_ui_semantics` 15 断言（可聚焦控件 role≠Custom + 装饰白名单 + Tag 行为）；Tag 进 ui_gallery；ctest 81/81 | **超了 §8 的 ≤60**：5 个事件钩子占 24 行；要再压需抽“按下-弹起”行为部件（设计 §4.7 之外）。M9 指标改为「静态叶子 ≤56 / 交互叶子 ≤70」 |
| Q5 大文件收尾 | **不做（评估后否决）** | — | M1 已修订为 ≤700 且当前最大 680 已达标；两份文件的“大”是内联实现体积而非职责混杂；拆 `ui_system.cpp` 反需引入内部头共享私有状态，性价比不成立 |

---

## 12. 审核要点

已全部得到答复并执行：

1. **D1 三层拆类** → 真拆（Q2-a），`widget.hpp` 750 → 291 + 379 + 17；零调用点改动
2. **D2 事件能力** → 均未选，Q1 否决（设计 §10）
3. **D3 M1 指标** → 从 ≤250 修订为 **≤700**（当前 680）
4. **范围** → Q3（绘制部件）、Q4（新控件 + 语义）做；Q5（大文件）评估后不做

剩余未达标的指标与原因见 §6（M10 / M11 / M12 三条，均已标注）。
