# 20261026-absolute-zindex

<!-- 为什么改、边界与不属于本次的范围 -->

## 绝对定位与层叠顺序

### 为什么

现有容器只能顺序排布，没法表达「角标固定在卡片右下角」「浮层盖在某个控件上方」。
`UIRoot` 的 overlays 只解决全局弹层，容器内部仍是纯流式。需要两个能力：

1. **绝对定位**：子项脱离流，用自身坐标相对父内容区定位，不影响兄弟。
2. **z-index**：同层内显式控制绘制与命中的先后。

### API（Widget 层）

- `set_absolute(bool)` / `absolute()`：默认 `false`。为真时容器不把它计入流式尺寸，
  按 `{内容区原点 + x(), 内容区原点 + y()}` 放置；尺寸仍走 `Widget::measure_child`
  （因此支持百分比与宽高比）。
- `set_z_index(int)` / `z_index()`：默认 `0`。数值大的画在上面、命中优先。
- `std::vector<Widget*> paint_order()`：按 z 稳定升序（同 z 保持加入顺序）；
  没有任何非零 z 时直接按原顺序返回，避免额外排序。

### 顺序规则

- 绘制：同层内按 `paint_order` 升序（z 小的先画）；`overlays` 始终在 `children` 之上。
- 命中：同层内按 `paint_order` **降序**（z 大的先命中）；`overlays` 仍优先于 `children`。
- 更新（动画）与 Tab 收集使用同一顺序，保证行为一致。

### 接入范围

- `Vbox` / `Hbox` / `Grid` / `Wrap`：`measure` 跳过绝对定位子项（不计入尺寸），
  `arrange` 单独摆放它们；`append_to` 用 `paint_order`。
- `UIRoot`：`append_to` 的 children 与 overlays 各自按 `paint_order`；
  `layout_child` 本来就按 `x()/y()` 定位，无需改动。
- `UISystem`：`collect_hit_path` 与 `update_tree` 改走 `paint_order`（反向命中）。
- 其它容器（Panel / TabBar / Table 等）暂不排序，仍按加入顺序绘制——它们没有重叠场景。

### 验证

新建 `test/test_ui_absolute_zindex.cpp` 并注册：

- Vbox 中一个绝对定位子项：兄弟位置与总高不受影响，子项按 `(x, y)` 相对内容区放置。
- 绝对定位 + 百分比宽：尺寸按父内容区解析。
- Grid / Wrap 中绝对定位子项不占格、不参与折行。
- `paint_order`：z 升序稳定；全为 0 时保持原序。
- 绘制顺序：两个重叠子项，z 大的顶点在 DrawList 中靠后。
- 默认行为不变（现有 38 个测试全绿）。

gallery `Layout` 分区：一个卡片容器，右下角绝对定位角标（徽标），说明不参与流式布局。

### 边界（本次不做）

- 不做 `position: fixed`（相对窗口）与相对最近定位祖先的 CSS 语义；绝对定位一律相对直接父容器的内容区。
- 不做 z-index 作用于 overlay 与 children 之间的跨界排序（overlays 永远在上）。
- 不做负 z-index 的特殊语义（只做数值排序）。
