# UI 层架构冗余评审（2026-10-29）

> 范围：`src/ui`（11688 行，58 个文件）
> 基线：main@2f4b88ac，工作区 dirty（本次评审未改代码）
> 关联：`docs/review/cosine-v5-261029-144412-94701bde.md`（#5 全项目评估）、
> `.design/changes/20261029-ui-redundancy-audit/`、`.design/DECISIONS.log` #150
> 性质：**静态评审**（逐文件阅读 + grep 计数），未做运行时验证与像素对比

---

## 0. 结论摘要

UI 层的冗余主要不是「控件多、代码啰嗦」，而是**五个概念各自有两个以上的所有者**：

| 概念 | 所有者数量 | 现在的形态 |
|---|---|---|
| 滚动状态 | 3 | `ScrollView` 自管 / `ListView` 用 `ScrollState` / `TreeView` 自管 |
| 行列表行为 | 2 | `ListView` 提供 + `Table` 又写一遍（键盘、访问器、`content_height`） |
| 浮层与模态 | 2 套机制 × 4 个所有者 | `UIRoot::push_modal` vs `add_overlay`；`DropdownScrim` / `MenuScrim` / `DialogHost` / 子菜单各自管生命周期 |
| 文本度量 | 8–10 处 | 每个控件各写 `atlas ? measure(...) : size * 0.5f` |
| 布局轴 | 2 份镜像 | `vbox.cpp` 200 行 / `hbox.cpp` 199 行，主轴不同其余相同 |

由此产生三个可观测后果：

1. **行为会静默分叉**：三种遮罩透明度已是 `0` / `0.05` / `0.55`，说明共享件抽出后旧路径仍在演化；
2. **契约空转**：`ListView::draw_row` 是纯虚，唯一实现为空函数，没有任何真实调用者；
3. **规则没有修改点**：「Esc 关闭最上层浮层」要在 4 处各写一次，滚轮的 `tau` 与吸附阈值要在 3 处各调一次。

改进方向不是「再抽几个基类」，而是**每个概念指定唯一所有者，然后删掉影子实现**。

---

## 1. 判定标准

评审中把「看起来像重复」的代码分成四类，避免把合理差异误判为冗余：

| 类别 | 判据 | 处理 |
|---|---|---|
| **架构冗余** | 同一概念有多个实现，且规则需要多处同步修改 | 指定唯一所有者，删影子实现 |
| **死契约** | 抽象存在但无真实调用者，或所有实现都是空 | 要么兑现，要么删除 |
| **机械重复** | 纯计算片段逐字复制，无语义差异 | 抽无状态工具函数 |
| **合理差异** | 语义不同（行距版 vs 切片版测量、窗口标题栏 vs 折叠头） | 保留，写清边界 |

对应地，以下**不算**冗余，本次不动：像素 `round` 对齐、`pressed_` 生命周期（按钮触发 / 勾选切换 / 单选组语义不同）、`Theme` 令牌表、渲染裁剪栈。

---

## 2. 五个根因

### 2.1 抽象只抽了一半：影子实现

公共件已经存在，旧路径也还活着：

**滚动**——`ScrollState` 的参数与公式（`tau`、吸附 `0.25`、Fast `0.5`、`idle > 0.1` 时 `tau *= 0.5`、`1 - exp(-dt/tau)`）在三个文件里逐字重复：

| 位置 | 形态 |
|---|---|
| `scroll_state.hpp:54-90` | 一维状态机（唯一被认可的公共件） |
| `scroll_view.cpp:265-286` | 私有 `offset_/target_/idle_`，双轴各判一次吸附 |
| `tree_view.hpp:301-327` | 私有 `scroll_/scroll_target_/idle_` |

**遮罩**——`scrim.hpp:9` 注释写着「遮罩绘制只有这一份实现」，绘制确实统一了（`append_scrim`），但**交互层没有**：`DropdownScrim`（`dropdown.hpp:24`，透明）、`MenuScrim`（`menu_bar.hpp:68`，5% 黑）、`DialogHost`（`dialog.hpp:197`，0.55 黑）各自实现「吞点击 / dismiss / 滚轮关闭」。

**列表**——`ListView` 抽出后，`Table` 仍保留自己的键盘处理与滚动访问器（见 2.2）。

架构后果是**两个真相源**。当前参数还一致，说明这是复制而非演化；一旦有人只改一处，三种滚动容器的手感就会分叉，而现有测试不会发现——因为没有「它们必须一致」的断言。

### 2.2 继承被当成命名空间

```cpp
// list_view.hpp:49  唯一要求子类实现的函数
virtual void draw_row(DrawList&, std::size_t, Rect, bool) const = 0;
// list_view.hpp:57  基类必须给 append_to 一个空实现
void append_to(DrawList&) const override {}
```

```cpp
// table.hpp:76  唯一的实现是空的
void draw_row(DrawList&, std::size_t, Rect, bool) const override {}
```

基类里唯一要求子类实现的函数**没有真实调用者**。`Table` 确实复用了基类的 `on_scroll / on_update / ensure_selected_visible`（这是真收益），但同时：

- `on_key_down`（`table.hpp:235`）与基类（`list_view.hpp:86`）是同一组 Up/Down/Home/End 的等价实现；
- 重声明 `scroll_offset / set_scroll_mode / set_scroll_smoothing`（非虚，只是**名字遮蔽**）；
- 私有 `content_height()`（`table.hpp:269`）遮蔽基类同名函数。

这是「为了借几行代码而建立 IS-A 关系」。它给下一个列表控件传递了错误模板：要么实现一个没人调的 `draw_row`，要么像 `Table` 一样用空实现绕过。

而 `TreeView` 不继承是**对的**——它的选中是 `vector<int>` 路径而非行号。这恰好证明：可复用的单位是「滚动状态 + 行绘制」，不是这一棵继承树。

### 2.3 同一概念多个所有者

| 概念 | 现状 | 证据 |
|---|---|---|
| 模态与浮层 | **两套机制并存**：`push_modal` 往 `children_` 塞 Scrim（Window）；`add_overlay` 由 4 个所有者各自管 scrim、z、销毁 | `root.hpp:19-27`、`window.hpp:81`、`dropdown.hpp:151`、`menu_bar.hpp:496,625`、`dialog.hpp:251` |
| 容器壳 | `Panel : Vbox`、`ScrollPanel : Panel`、`Section` 自持 Vbox 并转发 `add/children` | `panel.hpp:12`、`scroll_panel.hpp:14`、`section.hpp:19` |

浮层这一项最严重：`Esc 关闭最上层`、`点击外部关闭`、`浮层不越界`、`模态时下层不 hover` 这些规则，每条都要在 4 个地方各写一遍。已有的漂移就是三种遮罩透明度。

### 2.4 契约没有执行点

`Widget::measure_child`（`widget.hpp:269`）注释写着「容器测量子项的统一入口」，但：

- `ScrollPanel::measure` 自己算 padding、硬编码 `1e8/1e9`（`scroll_panel.hpp:45-56`）；
- `TabView` 直接 `child->measure()`（`tab_view.hpp:112`）；
- `DialogHost` 直接 `dialog_->measure()`（`dialog.hpp:182`）。

后果：`size_percent` / `aspect_ratio` 在这三个容器里**静默失效**——不报错、不崩溃，只是不生效。

同类问题：部分 `measure()` 读 `global_`。`Table` 已因 NaN 事故修过（DECISIONS #108），但「measure 只测量、不依赖 arrange 顺序」目前是**约定**而非测试。约定会被下一个人打破。

### 2.5 算法以镜像代码存在

`vbox.cpp` 200 行 / `hbox.cpp` 199 行，measure、arrange、flex 分配、margin 累加是同一套逻辑换主轴。`grid.cpp`（`164/197/252`）与 `wrap.cpp`（`72/184/208`）又各自重复「跳过 absolute 子项、累加 margin、调 `measure_child`」。

「轴」本该是参数，却被写成了两份代码——修一个轴的 bug 要在另一轴再修一遍。

---

## 3. 证据清单

| 编号 | 事实 | 位置 |
|---|---|---|
| A1 | 平滑滚动三份，参数与公式逐字相同 | `scroll_state.hpp:54-90`、`scroll_view.cpp:265-286`、`tree_view.hpp:301-327` |
| A2 | `Table` 重复 `on_key_down`、3 个滚动访问器、遮蔽 `content_height`，`draw_row` 空实现 | `table.hpp:76,235,269`、`list_view.hpp:86` |
| A3 | 三个全屏遮罩，alpha 分别为 0 / 0.05 / 0.55 | `dropdown.hpp:24`、`menu_bar.hpp:68`、`dialog.hpp:197`、`scrim.hpp:9` |
| A4 | `push_modal` 与 `add_overlay` 两套模态机制，`add_overlay` 生命周期由 4 个所有者自管 | `root.hpp:19-27`、`window.hpp:81`、`dropdown.hpp:151`、`menu_bar.hpp:496,625`、`dialog.hpp:251` |
| B1 | 文本宽度 fallback 复制（含按码点数计算的变体） | `button.hpp:36`、`checkbox.hpp:34`、`radio.hpp:44`、`label.hpp:93`、`tab_bar.hpp:160`、`menu_bar.hpp:428`、`table.hpp:300`、`dropdown.hpp:303`、`text_input.hpp:296` |
| B2 | `Checkbox::measure` 与 `Radio::measure` 逐字相同 | `checkbox.hpp:30-43`、`radio.hpp:40-53` |
| B3 | `find_root()` 四份：向上 `dynamic_cast<UIRoot*>` | `dropdown.hpp:315`、`window.hpp:455`、`menu_bar.hpp:419`、`dialog.hpp:234` |
| B4 | 水平项测量与命中：`text_width` + `padding*2` + 线性 `index_at` | `tab_bar.hpp:154-181`、`menu_bar.hpp:424-440` |
| B5 | 行高亮、斑马纹、逐行裁剪、滚动条 track/thumb 绘制 | `table.hpp:170-200,349`、`tree_view.hpp:224-232,500-520,577`、`scroll_view.cpp:309` |
| B6 | 时间↔像素映射与刻度候选 | `timeline_view.hpp:450-454`、`preview_ruler.hpp:70-105` |
| C1 | Vbox/Hbox 镜像 | `vbox.cpp`、`hbox.cpp` |
| C2 | Grid/Wrap 重复绝对定位与 margin 处理 | `grid.cpp`、`wrap.cpp` |
| C3 | 三种「带壳容器」 | `panel.hpp:12`、`scroll_panel.hpp:14`、`section.hpp:19` |
| D1 | 三处容器绕过 `measure_child` | `scroll_panel.hpp:45-56`、`tab_view.hpp:112`、`dialog.hpp:182` |
| D2 | `measure()` 读 `global_`，纯度靠约定 | 多处，`table.hpp` 已修（#108） |
| D3 | `ListView` 定位不清（行为基元 vs mixin） | `list_view.hpp` |
| D4 | `ScrollPanel`/`Section` 是 facade 还是独立控件不清 | `scroll_panel.hpp`、`section.hpp` |

复现方式（只读）：

```bash
grep -rn "1.0f - std::exp(-dt" src/ui          # A1 三份
grep -rn "dynamic_cast<UIRoot" src/ui          # B3 四份
grep -rn "0.5f;" src/ui/*.hpp | grep -i size   # B1 fallback
wc -l src/ui/vbox.cpp src/ui/hbox.cpp          # C1 200 / 199
```

---

## 4. 与 #5 评估的关系

本文是 #5「3. 剩余重复」的展开与修正，不是重复评估。

| #5 结论 | 本次结论 | 处理 |
|---|---|---|
| 3.1 `ListView` 抽象尚未闭合；遗留任务 **UI-10** 为「`draw_row` 接管 + `TreeView` 迁移」，前置是渲染快照对比 | **不建议迁移 TreeView**。它的选中是路径，不是行号；应删除 `draw_row` 纯虚，让基类只管行为。这样 UI-10 的前置条件（`round` 与 `floor` 取整口径）直接消失 | 建议**重定义 UI-10**，需用户批准后改 `.design/lines/ui.md` |
| 3.2 滚动实现 2 套；`ScrollState` 定位收敛为「一维、给列表用」；扩成二维会让它变成什么都装的对象 | 实际有 **3 处**平滑实现（`TreeView` 是第三份）。同意不扩成二维：用**两个 `ScrollState` 实例**表达双轴，保持一维类型 | 与 #5 同向，只是补齐第三份 |
| 3.4 `Layer` 已声明但未驱动命中排序，重写属「为一致性冒回归风险」 | 同意。浮层统一只收拢 scrim/生命周期，**不碰**命中顺序与 `Layer` | 维持 |
| 6.1 `Widget` 承担 12 种职责，已决定不拆 | 同意，不在本次范围 | 维持 |

---

## 5. 改进方案

原则：**每个概念只允许一个实现、一个修改点**；其余位置只能调用，不能重写。

### Move 1（P0）滚动 → `ScrollState` 是唯一状态机

- `ScrollView`：删掉 `offset_/target_/idle_/smoothing_`，换成两个 `ScrollState`（x、y）；`place_content()` 读两者 offset；
- `TreeView`：删掉 `scroll_/scroll_target_/idle_`，换成一个 `ScrollState`（变量名保留 `scroll_` 以缩小 diff）；
- 模式、平滑、吸附、Fast 加速只在 `ScrollState` 里存在。

必须保留的既有约束：

- 平滑更新**不许** `mark_layout_dirty()`（DECISIONS #107）；
- 像素取整留在绘制侧，不塞进 `ScrollState`（DECISIONS #79）。

删除量约 60 行。风险：`ScrollView` 双轴独立性、Line 模式行步进。

### Move 2（P0）列表 → 基类只管行为

推荐**删除** `draw_row` 纯虚，而不是逼 `Table` 去实现。理由：Table 是「表头 + 多列 + 斑马纹」，TreeView 是「缩进 + 路径」，画法本就不该由基类规定；硬造一个 `Rect row` 参数只会让子类在函数里重算列宽。

具体动作：

- 删除 `ListView::draw_row` 与 `Table` 的空实现；
- 删除 `Table::on_key_down`、`scroll_offset`、`set_scroll_mode`、`set_scroll_smoothing`、`content_height`；
- 把 `scroll_mode() / scroll_smoothing() / scroll_target()` 三个只读 getter 上移到基类；
- 保留基类的 `on_scroll / on_update / ensure_selected_visible`（真复用）。

删除量约 40 行。风险：测试若直接引用 `Table::scroll_offset` 等成员需调整（签名不变，风险低）。

### Move 3（P1）浮层 → `UIRoot` 是唯一所有者

```cpp
struct OverlayOptions {
    float scrim_alpha;                               // 来自 Theme token
    bool  dismiss_on_outside;
    bool  dismiss_on_scroll;
    bool  dismiss_on_escape;
    std::function<void(glm::vec2)> on_outside_hover; // 菜单栏切换需要
};
OverlayHandle UIRoot::show_overlay(std::unique_ptr<Widget> content, OverlayOptions);
```

- 句柄负责：创建/移除 scrim、z 分配、延迟销毁（复用已有 `trash_`）、Esc 关闭最上层；
- Dropdown / Menu / Dialog 删掉各自的 Scrim 类，只提供内容控件；
- `MenuScrim` 独有的「hover 转发给菜单栏」用 `on_outside_hover` 保留——这是抽公共件最容易丢的语义，必须显式留在接口上；
- `DialogHost` 退化为「居中 + clamp 布局」，不再画遮罩；
- 透明度进 `Theme`（两档 token），不再有字面量。

删除量约 120 行。风险：点击穿透、z 顺序、菜单 hover 转发。

### Move 4（P0/P1）工具层 → 每个只留一个函数

| 工具 | 收掉谁 | 落点 |
|---|---|---|
| `text_width(text, size)` | B1 的 9 处 fallback | 新 `ui/text_metrics.hpp`（内部用 `FontAtlas::measure_width`） |
| `Widget::root()` | B3 的 4 份 `find_root` | `widget.hpp` |
| `HorizontalItems`（宽度缓存 + `index_at`） | TabBar / MenuBar | `ui/horizontal_items.hpp` |
| `TimeRuler`（刻度候选、`x_at/time_at`） | TimelineView / PreviewRuler | 纯函数，可单测 |
| `RowPainter` / `ScrollBarPainter` | Table / TreeView / ScrollView 的行高亮、斑马纹、track/thumb | `ui/painters.hpp` |

边界：`Label`（带行距）与 `TextInput`（按切片、按码点数）的测量语义不同，**不并入**。竖排居中已由 `FontAtlas::centered_text_y()` 收口，本次只收敛宽度。

删除量约 60（工具）+ 80（绘制器）行。

### Move 5（P2）布局 → 一维内核 + 工具

- `LinearLayout(axis)` 承载 measure/arrange/flex/margin，`Vbox`/`Hbox` 退化为薄封装；
- `layout_utils` 提供「绝对子项摆放」「流式游标」，Grid/Wrap 复用。

前置条件：**先写对偶测试**——同一组子项宽高互换后结果必须对称。先把现状钉住再合并，否则 flex/margin 的细微分叉会变成视觉回归。

删除量约 250 行。风险最高（flex_grow、margin、cross-align 的细节分叉）。

---

## 6. 执行顺序与依赖

依赖关系决定顺序，不按代码行数排：

```
契约测试（measure 纯度）
   ├─ root() ─────────────► 浮层统一（show_overlay 需要 root）
   ├─ text_width ─────────► Checkbox/Radio/TabBar 收敛
   ├─ 滚动（ScrollState）─► TreeView/Table 的行绘制收敛
   └─ ListView 清理 ──────► 绘制器抽取（先定行为边界）
布局内核 ──────────────────► 最后：行为收益最低、回归面最大
```

理由：`find_root` 必须在浮层之前（否则统一后的 `show_overlay` 还得抄一遍向上查找）；文本度量必须在 checkbox/radio 之前；滚动必须在行绘制器之前（行位置依赖 offset 语义）。

---

## 7. 验收手段

评审阶段只有静态证据，落地阶段需要三类**可执行**检查：

1. **收敛轨迹一致**：同一组滚轮输入 + 同一串 `dt`，`ScrollView` / `Table` / `TreeView` 的 offset 序列逐帧相等；
2. **measure 与 arrange 顺序无关**：`global_` 全 0 时与 arrange 后再 measure 结果一致（同时覆盖 D1 的三处绕过）；
3. **两轴对称**：同一组子项宽高互换后，Vbox/Hbox 的 measure/arrange 结果对称。

外加：`ctest` 全绿、ASAN 冒烟无回归、遮罩深浅与行高亮、滚动条，均做目视抽查。

---

## 8. 明确不做

- 不新增控件、不改视觉语言、不动 `Theme` 既有令牌值（只新增遮罩 token）；
- 不引入布局引擎、不做 CSS 式约束系统；
- 不重写渲染批次/裁剪栈，不改 `Layer` 与命中顺序；
- 不拆分 `menu_bar.hpp`（857 行属菜单语义的合理复杂度）；
- 不删除公开 API，除非仓库内零调用且已列清单。

---

## 9. 待拍板

| # | 问题 | 建议 | 影响 |
|---|---|---|---|
| 1 | `draw_row` 纯虚：删除，还是让 Table 实现？ | **删除**，基类只管行为 | 决定 UI-10 是否重定义 |
| 2 | `push_modal` 与 `add_overlay`：合并，还是两层共享 z/销毁？ | **保留两层语义**，共享 `OverlayHandle` | 决定浮层改造范围 |
| 3 | 遮罩透明度：两档 token，还是统一一档？ | **两档 token**（菜单轻、模态重） | 纯视觉，改动小 |
| 4 | 双轴滚动：两个实例，还是扩成 `vec2`？ | **两个实例**（与 #5 结论一致） | 决定 `ScrollState` 是否变形 |
| 5 | `Section`/`ScrollPanel`：保留转发 API，还是要求显式取 `content()`？ | 保留转发，但文档标注为 facade | 仅影响 API 说明 |

第 1、2 项会改变 `.design/lines/ui.md` 里 UI-10 的定义，需用户批准后才能改。

---

## 10. 量化基线

| 指标 | 值 |
|---|---|
| `src/ui` 行数 | 11688（58 文件） |
| 最大文件 | menu_bar 857 / tree_view 594 / window 555 / timeline_view 554 / text_input 549 |
| 平滑滚动实现 | 3（`ScrollState`、`ScrollView`、`TreeView`） |
| 全屏遮罩实现 | 3（alpha 0 / 0.05 / 0.55） |
| 浮层生命周期所有者 | 4（Dropdown / MenuBar / 子菜单 / Dialog） |
| `find_root` 副本 | 4 |
| `ListView` 的真实子类 | 1（`Table`），其 `draw_row` 实现为空 |
| 预计增删 | 删除约 500–600 行，新增约 180 行（净减约 400） |

---

## 11. 方法说明与置信度

| 结论 | 依据 | 置信度 |
|---|---|---|
| 滚动三份实现参数一致 | 逐行比对三处代码 | 高 |
| `draw_row` 无真实调用者 | `grep` 全仓 + 阅读 `append_to` | 高 |
| 三处容器绕过 `measure_child` | 阅读 `measure` 实现 + `grep measure_child` | 高 |
| `size_percent` 在这三处失效 | 由上一行推理，**未运行验证** | 中——落地前需用例确认 |
| Vbox/Hbox 可合并 | 行数与结构比对，**未做行为等价验证** | 中——必须先补对偶测试 |

#5 的教训是「先写测试再动手比读代码估范围可靠」。本文所有改动建议都以「先补对应测试」为前置，尤其 Move 5。
