# 20261026-progress-submenu

<!-- 为什么改、边界与不属于本次的范围 -->

## 进度与子菜单

### Progress 不确定态

- `set_indeterminate(bool)` / `indeterminate()`：切换确定/不确定两种表现，默认确定。
- 动画时间用已有的 `Widget::on_update(dt)`（由 `UISystem::update_tree` 每帧驱动），
  推进 `phase_ ∈ [0, 1)`，周期固定 1.2s；仅在 `indeterminate()` 为真时推进。
- 绘制：`Track` 角色轨道 + 一条宽 30% 的 `accent` 条带，位置 `x = (w + band) * phase - band`，
  即从左外滑到右外循环；条带两端圆角用 `radius_pill`（classic 为直角）。
- 确定态绘制不变；切回确定态时保留 `phase_`（下次再进入不确定态从上次位置继续）。
- 不确定态不显示百分比文本（本来就没有）。

### 菜单子菜单（多层）

- `MenuItem::children`（`std::vector<MenuItem>`，默认空）+ 工厂 `MenuItem::submenu(label, children, enabled)`；
  `has_submenu()` = `!children.empty()`。递归结构天然支持任意层数。
- `MenuPopup` 改为持有自己的 `items` **拷贝**（`set_items`）：顶层来自 `MenuBar` 的菜单，子层来自父项 `children`。
  菜单项数量小，拷贝换来地址稳定与生命周期简单。
- `MenuBar` 用 `std::vector<std::unique_ptr<MenuPopup>> popups_` 维护层级栈：
  `popups_[0]` 是顶层菜单，`popups_[1..]` 是逐层打开的子菜单。
- 绘制：有子菜单的行右侧画右向三角（`graphics::Triangle`），快捷键文本让位。
- 交互：
  - 悬停到「有子菜单」的行 → 关闭该层之后的所有子菜单，再打开新子菜单；
  - 悬停到无子菜单的行 → 关闭该层之后的子菜单；
  - 定位：子菜单 `x = 父 popup 右边 - 4`；右边界不足时翻到父 popup 左侧；`y = 该行顶部`，并 clamp 到窗口内；
  - 点击项：有子菜单则打开子菜单；否则执行 `action` 并关闭整条链；
  - `Escape` / 点击菜单外 → 关闭整条链。
- 键盘（作用于 `active_depth_ = popups_.size() - 1` 这一层）：
  - `Up`/`Down`/`Home`/`End` 在当前层移动高亮；
  - `Right` 进入子菜单（高亮项有子菜单时）；`Left` 退回父层；
  - `Enter`/`Space`：有子菜单则进入，否则执行 `action` 并关闭全部；
  - `Escape` 关闭全部。
- 每层的高亮：`MenuPopup` 自己持有 `hovered_`（鼠标）与 `keyboard_highlight_`（键盘），
  绘制时鼠标优先、否则用键盘高亮，不再依赖 `MenuBar` 的单一 `highlight_`。
- 顶层菜单之间的 Left/Right 切换保留（在 `active_depth_ == 0` 且当前项无子菜单时生效）。

### 验证

`test_ui_progress`：

- 不确定态：`tick(dt)` 后 `phase` 增加，条带位置随之变化；不同 `dt` 序列可复现。
- 确定态：`phase` 不推进，绘制与迁移前一致（顶点数不变）。
- 顶点数：不确定态 > 0，且条带在两端不越出控件裁剪区。

`test_ui_submenu`：

- `MenuItem::children` 结构与 `has_submenu`。
- 悬停有子菜单的行 → overlay 数量 +1；悬停无子菜单的行 → 回到原数量。
- 导航到第二层：继续悬停另一带子菜单的项 → overlay 数量按层增加。
- 键盘：`Right` 进入、`Left` 退回、`Enter` 执行并全关；多层时 `Left` 只退一层。
- 点击子菜单项 → `action` 被调用一次，且所有 overlay 关闭。
- `Escape` 关闭整条链。
- 子菜单项的 `action` 与快捷键文本不影响布局（右三角与快捷键不重叠）。

gallery：

- `Inputs` 分区新增 indeterminate Progress（gallery 主循环已传真实 `dt`）。
- `Menu` 分区增加一个多层子菜单示例（如 `Edit → Advanced → Text → …`），用于人工验收。

### 边界（本次不做）

- 子菜单支持任意层数；不做子菜单的打开/关闭动画（与「面板/弹层出现消失过渡」一起留到以后）。
- 不做 `Progress` 的百分比文本、不做条带走动的缓动曲线（匀速）。
- 不做子菜单的虚拟滚动（项过多时不做滚动，只 clamp 到窗口内）。
