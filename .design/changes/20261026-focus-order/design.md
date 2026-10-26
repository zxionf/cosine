# 20261026-focus-order

<!-- 为什么改、边界与不属于本次的范围 -->

## 键盘导航

### 现状

- Tab 顺序完全由 `UISystem::collect_focusable` 的深度优先树序决定（overlay 先于 children），控件无法调整。
- `TabBar` 已有 Left/Right/Home/End 切换；`Dropdown` 已有 Up/Down/Home/End/Enter/Esc。
- `RadioGroup` 没有方向键，且组内每个 `Radio` 都是独立的 Tab 停靠点 —— 与「单选组只占一个 Tab 停靠点、组内用方向键切换」的标准语义不符。

### 设计

**1. Tab 顺序可配置**

```cpp
// Widget
void set_tab_order(int order);
int tab_order() const;
```

- `> 0`：显式顺序，数值小的先；建议 1..9999。
- `0`（默认）：按树序，与现状一致。
- `< 0`：不参与 Tab 遍历（仍可鼠标点击聚焦，也可被 `set_focus` 程序聚焦）。

排序规则：`collect_focusable` 先按树序收集，再 `std::stable_sort`：

```text
键 = (order > 0) ? order : kAutoOrder   // kAutoOrder = 1'000'000
```

于是显式顺序整体排在「按树序」的控件之前，同类内部保持树序（`stable_sort` 不改变相对位置）。

**2. RadioGroup 组内导航（roving tabindex）**

- `RadioGroup::focusable()` 返回是否已有选项。
- `set_options` 时给每个 `Radio` 设 `tab_order(-1)`：整个组只占一个 Tab 停靠点。
- 方向键：`Up`/`Left` 上一项，`Down`/`Right` 下一项，`Home`/`End` 首/尾；
  切换时调用 `select(index, true)` 并把「组内焦点标记」移到新选项。
- `on_focus_gained` 把焦点标记放到当前选中项；`on_focus_lost` 清理（并调用基类）。
- 焦点框仍由 `Radio` 自己的 `append_focus_ring` 绘制，因此与 `FocusStyle`/`FocusPolicy` 自动一致。

**3. 不改的东西**

- `TabBar` / `Dropdown` 的方向键已具备，本次不动。
- 程序性焦点（`set_focus`）不受 `tab_order < 0` 限制，便于弹窗默认焦点等场景。

### 验证

`test_ui_focus_order`（纯逻辑）：

- 默认树序：与迁移前一致（回归保护）。
- 显式顺序 3/1/2 → Tab 顺序 1→2→3，Shift+Tab 反向。
- `tab_order < 0` 的控件被 Tab 跳过，但 `set_focus` 仍可聚焦。
- 混排：显式 order 的控件排在未设 order 的控件之前；同 order 保持树序。
- RadioGroup：Down/Up/Home/End 切换选中且"组内焦点"跟随；组内 Radio 的 `tab_order < 0`；
  Tab 从组直接跳到下一个控件（组只占一个停靠点）。
- 现有 31 个测试全部通过；ASAN 通过。

gallery：`Inputs` 分区新增 `Tab order (3 → 1 → 2)` 三个按钮，验证 Tab 实际顺序；
Radio group 标题补「Up/Down 切换，组只占一个 Tab 停靠点」。

### 边界（本次不做）

- 不做焦点陷阱 / 焦点环（modal 已通过 overlay 优先级实现类似效果）。
- 不做任意容器的方向键焦点移动（只 RadioGroup；TabBar/Dropdown 已有）。
- 不做焦点记忆（切回窗口恢复上次焦点）。
- 不引入 `tab_order` 的持久化或运行时重排（顺序在布局树变化后按同一规则重算）。
