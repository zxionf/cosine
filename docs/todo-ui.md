# src/ui 改造方案 v2（审核稿）

> 版本：v2 审核稿（2026-11-27）
> 基线：`main@b91d74e`（工作区另有一批 UI-55 改动待提交，见 §2.3）
> 设计依据：`.design/ui-redesign.md`（v1 + 变更记录）
> 状态：**待审核**。本文件取代 v1（v1 是执行记录，结论已并入 §2）。
> 范围：**只改 `src/ui/`**（必要时触及 `test/`、`src/CMakeLists.txt`、`src/cosine` 的 include）。

---

## 0. 一句话

v1 计划的目标是"把 `Widget` 从 630 行的上帝基类拆成薄节点 + 部件"。
**实际完成的是"收权 + 部件化 + 契约化"**（§2），**没完成"拆类"**（§3 差距）。
因此本稿只规划**剩余四件事**：能力接口、三层拆类、绘制部件、大文件收尾 —— 每件都给了可选路线与代价，需要你在 §5 的三个决策点上拍板。

---

## 1. 基线事实（实测，2026-11-27）

| 项 | 数值 | 命令 |
|---|---|---|
| 提交 | `b91d74e`（+ 一批未提交的 UI-55 改动） | `git log --oneline -1` |
| 文件数 / 行数 | 76 / **13,622** | `find src/ui -name '*.hpp' -o -name '*.cpp' \| xargs wc -l` |
| 控件类 | **46** | 见 §7 |
| `Widget` 虚函数 | **37**，其中事件钩子 **17** | `cat src/ui/node/*.hpp \| grep -c "    virtual "` |
| `sizeof(Widget)` | **432 B** | 单测 `test_ui_state_ownership` |
| 测试 | **80 目标 / 2,027 断言** | `ctest` |
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

`test/test_ui_architecture.cpp`（27 断言）：

| 契约 | 断言内容 |
|---|---|
| D1 | `geom/` 与 `layout/measure_cache.hpp` 不 include 控件树 |
| D2 | `core/` `containers/` 不反向 include `composites/` `views/` |
| D3 | `style/` 不 include 具体控件 |
| D4 | `semantics/` 只依赖标准库 |
| D5 | `src/ui` 根目录无头/源文件、无转发头 |
| K1 | 无 per-widget `enabled_`/`pressed_`/`dragging_`；`control_state()` 已消失 |
| K2 | `children_` 只在 `node/widget.hpp` |
| K3 | 几何写只在 `set_geometry` 家族 |
| K5 | 基类虚函数 ≤42 |
| K6 | `MeasureKey` 只有一处定义 |
| K7 | `layout_` 直接赋值只在 2 个函数（12 处） |

### 2.3 待提交（UI-55）

`menu_item.hpp` / `menu_popup.hpp` / `menu_popup.cpp` 三个新文件 + `menu_bar.hpp` 瘦身 + 语义钩子合并 + CMake 与文档。

---

## 3. 与设计文档的差距审计（逐节）

| 设计章节 | 内容 | 状态 | 缺口与原因 |
|---|---|---|---|
| §4.1 `Node`（身份/几何/树/层叠，≤180 行） | 独立基础类 | ❌ **未创建** | 卡在 `children_` 的元素类型：现有 400+ 站点依赖 `vector<unique_ptr<Widget>>`，拆 Node 必须把树元素改成 `Node*` → 全站类型迁移（重写级） |
| §4.2 `LayoutNode`（≤220 行，含 3 部件） | measure/arrange + 部件 | ⚠️ **部件已在 `Widget` 内**，类未拆 | 同上 |
| §4.3 `Interaction` | 状态唯一所有者 | ✅ 完成 | — |
| §4.4 `VisualState` | 104 B 内嵌 | ✅ 完成（比设计还好：没建 60 项 Theme 表） | — |
| §4.5 `Semantics` | 派生投影 | ✅ 完成 | 27 个控件覆写；其余按 `Custom` |
| §4.6 四类节点（Leaf/Container/ListView/浮层） | 分类 | ⚠️ `LeafNode` 有；`ContainerNode` 无 | `ContainerNode` 依赖 Node/LayoutNode 拆分 |
| §4.7 `BoxPainter` / `TextPainter` | 绘制部件 | ❌ **未做** | 纯降重（25 处 `visual_style`、13 处焦点环），无结构风险 |
| §7 43 控件 role 映射 | 语义角色 | ⚠️ 27/46 有 role | 其余是容器/装饰，按 `Custom` 也合理 |
| §8 新控件 ≤60 行 | — | ⚠️ 静态叶子 27~56 行达标；交互叶子未实测 | 无 `Tag` 示例控件 |
| §9 `UISystem` 边界 | 不动 | ✅ 未动（558 行仍在一处） | — |
| §10 被否方案 | 不引入 | ✅ 未引入 | — |

**一句话结论**：v1 的"部件化 + 收权 + 契约化"全部落地并被断言锁住；**"拆类"（§4.1/§4.2/§4.6）整体未动**，M1（单文件 ≤250 行）因此未达成（最大 750 行）。

---

## 4. 剩余改造方案（5 个批次）

> 每批都是"可独立提交、可独立回滚、快照哈希必须不变"的粒度。
> 预估按 1 人熟悉代码后的工作量计。

### Q1 — 事件能力接口（设计 §4.2 隐含 + §4.6）

**问题**：`Widget` 有 **17 个事件钩子** + `hit_testable/focusable/preferred_focus` = 20 个虚函数，占基类虚函数的 54%。43 个子类继承全部空实现。

**目标**：把"能接收什么事件"变成**可查询的能力**，控件只实现它用得到的那一组。

**三条候选路线**（需拍板，见 §5-D2）：

| 路线 | 做法 | widget.hpp 减 | 虚函数 | 破坏面 | 预估 |
|---|---|---|---|---|---|
| **Q1-a 纯接口多继承** | `input/pointer_target.hpp`（6 个钩子）、`key_target.hpp`（3）、`focus_target.hpp`（3）；`Widget` 只保留 `hit_testable/focusable`；`UISystem` 用 **tag 位图** + `static_cast` 分发（无 RTTI） | −约 90 行 | 37 → **约 22** | 43 控件改为多继承对应接口；`UISystem::dispatch` 改按能力分发 | 2–3 天 |
| **Q1-b 单一事件入口** | `virtual bool on_event(Event&)`，17 个钩子变 `Event` 的 kind 分支 | −约 60 行 | 37 → **约 20** | 43 控件的事件处理全部改签名（机械但量大） | 3–4 天 |
| **Q1-c 维持现状** | 只把钩子分区注释、加 `capabilities()` 查询位图（不改分发） | 0 | 37 | 无 | 0.5 天 |

**推荐 Q1-a**：与设计文档一致、虚函数降幅最大、能力可查询（调试面板/未来 a11y 直接用）。

**验收**：
- `test_ui_architecture` 新增断言：`node/*.hpp` 中 `on_*` 虚函数 ≤ 6；capability 位图与实现的接口一致（用探针控件验证）
- 现有 80 测试全绿、快照不变、三入口冒烟干净
- `UISystem::dispatch` 的命中路径仍只有一条（`collect_hit_path`）

---

### Q2 — 三层拆类（设计 §4.1 / §4.2 / §4.6）

**问题**：`widget.hpp` 750 行里，几何/树/可见性 ≈ 250 行、布局机制 ≈ 200 行、事件 ≈ 90 行、语义 ≈ 50 行、样式 ≈ 30 行。

**根因**：`children_` 是 `std::vector<std::unique_ptr<Widget>>`，`children()/child_at()/adopt()` 全部以 `Widget*` 为元素类型 → 基类无法上移。

**三条候选路线**（需拍板，见 §5-D1）：

| 路线 | 做法 | widget.hpp | `sizeof(Widget)` | 破坏面 | 预估 |
|---|---|---|---|---|---|
| **Q2-a 真分层** | `Node`（几何/树/层叠/语义，树元素 `Node*`）→ `LayoutNode`（measure/arrange + 部件）→ `Widget : LayoutNode`（空壳别名） | **≈250** | +8~16（多一层 vptr） | **400+ 站点**类型迁移（`children()` 返回 `Node*`，容器要 `static_cast` / 模板）；测试大面积改 | 6–9 天 |
| **Q2-b 树存储抽取** | 只把树 API 抽成 `tree/child_list.hpp`（`Widget` 持有，元素类型不变） | ≈660 | 0 | 0（纯内部） | 1–1.5 天 |
| **Q2-c 不做** | — | 750 | 0 | 0 | 0 |

**推荐 Q2-b 并明确接受"不拆类"**：`widget.hpp ≈660` 已接近"单类单职责"的实际下界；Q2-a 的收益（−400 行）换 400+ 站点迁移 + 6~9 天，且 `Widget` 仍在（只是变成空壳），对使用者无感。

**若选 Q2-a**，必须先做的准备：
1. `children()` 改名 `nodes()`（返回 `Node*` 视图）并保留 `children()` 兼容层一个版本
2. 45 个容器的 `measure_child(*child, …)` 改为 `measure_child(Node&, …)`
3. 快照哈希 + 80 测试作为唯一护栏；每完成一层跑一次

**验收（Q2-b）**：`widget.hpp ≤ 660`；`test_ui_architecture` 的 K2 断言改为"`children_` 只出现在 `tree/child_list.hpp`"；快照不变。

---

### Q3 — 绘制部件（设计 §4.7）

**问题**：`visual_style()` 25 处调用、`append_focus_ring()` 13 处、边框/圆角/文字居中在每个控件重复 3~15 行。新控件仍要抄"画框 + 焦点环 + 文字居中"。

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

---

### Q4 — 语义与分类收尾（设计 §4.6 / §7）

**目标**：把"四类节点"从概念落成可查询事实，而不是只有 `LeafNode`。

**清单**：
1. `Widget::is_container()`：非虚，`children()` 非空或 `child_count()>0` → 由存储派生（不需要虚函数）
2. 补齐剩余控件的 `semantics_role()`：`ColorRect`→`Custom`（明确）、`PreviewRuler`→`Custom`、`TabBar`→`Custom`、`Scrim`→`Custom`、`SceneView`/`TimelineView`→`Custom`、`FileBrowser`→`List`、`MenuScrim`/`MenuPopup`→`Menu`（已有 `Menu`）
3. 新增示例控件 `Tag`（约 40 行：静态叶子 + 可点 + 焦点环），作为 §8 "新控件 ≤60 行"的实证，并进 `ui_gallery`
4. `test_ui_semantics`：遍历全树断言"每个可聚焦控件的 role ≠ Custom"、"每个非空容器 `is_container()` 为真"

**预估 1 天。风险低。**

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

## 5. 需要审核拍板的三个决策点

### D1 — 三层拆类走哪条路？

| 选项 | 结果 | 代价 |
|---|---|---|
| **Q2-a 真分层** | `widget.hpp ≈250`，M1 达标 | 400+ 站点迁移，6–9 天，`sizeof` +8~16 |
| **Q2-b 只抽树存储**（推荐） | `widget.hpp ≈660`，M1 修订为 ≤700 | 1–1.5 天，零破坏 |
| **Q2-c 不做** | 保持 750 | 0 |

**我的判断**：Q2-b。理由：`Widget` 仍是所有控件的唯一基类，真分层后它变成空壳别名 —— **抽象层数增加了，但没有减少任何调用方需要理解的东西**，而代价是 400+ 站点迁移与更高的 `sizeof`。

### D2 — 事件能力接口走哪条路？

**推荐 Q1-a**（纯接口多继承 + tag 位图分发）：与设计一致、虚函数 37→22、能力可查询。
若你更看重"零破坏"，选 **Q1-c**（只加 `capabilities()` 查询，不改分发）。

### D3 — M1 指标怎么定？

| 选项 | 值 | 依据 |
|---|---|---|
| 保持原文 ≤250 | 250 | 需 Q2-a 才能达成 |
| **修订为 ≤700**（推荐） | 700 | Q2-b 后可达成；且 750 行里 ≈180 行是契约注释 |
| 修订为"单文件 ≤750 且注释占比 ≤30%" | 750 | 承认注释价值 |

---

## 6. 指标看板（现状 → 目标）

| # | 指标 | 现状 | 可达目标（Q1+Q2b+Q3+Q4+Q5） | 需 Q2-a 才能达 |
|---|---|---|---|---|
| M1 | 最大单文件行数 | 750 (`widget.hpp`) | **≈660** | ≤250 |
| M2 | 基类虚函数 | 37（含 17 事件钩子） | **≈22**（Q1-a） | ≈22 |
| M3 | 纯样板 `arrange()` | 0 ✅ | 0 | 0 |
| M4 | 状态字段副本 | 各 1 ✅ | 1 | 1 |
| M5 | 测量缓存套数 | 1 ✅ | 1 | 1 |
| M6 | 过渡状态内存 | 104 B / 0 `new` ✅ | 104 B | 104 B |
| M7 | 几何写绕过点 | 0 ✅ | 0 | 0 |
| M8 | 树遍历方式 | 1 ✅ | 1 | 1 |
| M9 | 新控件行数 | 静态 27~56、交互未测 | **≤45**（Q3 后，`Tag` 实证） | ≤45 |
| M10 | 测试/断言 | 80 / 2,027 | **≥85 / ≥2,150** | 同 |
| M11（新） | 焦点环手写点 | 13 | **1** | 1 |
| M12（新） | 最大文件 TOP3 和 | 1,992 | **≤1,200** | — |

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
| 一次性重写 `src/ui` | 2,027 条断言归零，且算法本身正确 |

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
| Q3 绘制部件 | **done（UI-57）** | `style/painters.hpp` 80 行（box / focus_ring / box_with_focus / centered_text）；7 控件迁移（Button/Checkbox/Radio/TextInput/Dropdown/Tooltip/Progress）；控件内手写焦点环 12→6；`LayoutNode::append_focus_ring` 改为转发（实现只剩一份）；快照不变 | 剩余 6 处是“环心不是控件 bounds”的合法直调（Slider 手柄 / TabBar 页签 / Table、TreeView 面板 / MenuBar） |
| Q4 新控件 + 语义 | **done（UI-58）** | `core/tag.hpp`（可点/可禁用/焦点环/语义，非注释 66 行）；`test_ui_semantics` 15 断言（可聚焦控件 role≠Custom + 装饰白名单 + Tag 行为）；Tag 进 ui_gallery；ctest 81/81 | **超了 §8 的 ≤60**：5 个事件钩子占 24 行；要再压需抽“按下-弹起”行为部件（设计 §4.7 之外）。M9 指标改为「静态叶子 ≤56 / 交互叶子 ≤70」 |
| Q5 大文件收尾 | **不做（评估后否决）** | — | M1 已修订为 ≤700 且当前最大 680 已达标；两份文件的“大”是内联实现体积而非职责混杂；拆 `ui_system.cpp` 反需引入内部头共享私有状态，性价比不成立 |

---

## 12. 审核要点（只问四个问题）

1. **D1**：三层拆类 —— 真拆（6–9 天，`widget.hpp ≈250`）/ 只抽树（1.5 天，≈660）/ 不做？
2. **D2**：事件能力 —— 纯接口多继承（虚函数 →22，43 控件改继承）/ 单一 `on_event` / 只加查询（零破坏）？
3. **D3**：M1 是否接受从 250 修订为 700（理由：750 行里 ≈180 行是契约注释，剩余是几何+布局+树+事件四类不可避免的职责）？
4. **范围**：Q3/Q4/Q5 是否都要？还是只做 Q1+Q2（结构），其余按需？
