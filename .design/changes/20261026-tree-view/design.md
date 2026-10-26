# 20261026-tree-view

<!-- 为什么改、边界与不属于本次的范围 -->

## TreeView

- 数据结构：`TreeNode { title, children, expanded }`；`set_roots` / `add_root` / `clear_roots`。
- 选中用「根索引路径」`std::vector<int>`（如 `{0, 2, 1}`），展开/折叠不改变选中的含义。
  `selected_path()` / `set_selected_path(path, notify)` / `clear_selection()` / `set_on_select`。
- 可见行 = 展开节点的前序遍历；`visible_rows()` 同时供绘制、命中与键盘导航使用。
- 绘制与 Table 同源：面板背景、行高 26、选中（`focus_ring` 半透明）/hover/斑马纹、
  自绘滚动条、整控件裁剪。
  - 每层缩进 `indent = 16`；有子节点的行绘制 chevron（展开下指、折叠右指），
    叶子行不画箭头但保留箭头列宽，保证同层文字对齐。
  - 文本在行内裁剪，超出右边界不越界绘制。
- 展开/折叠：`set_expanded(path, bool)` / `toggle(path)` / `expand_all()` / `collapse_all()`；
  点击箭头切换；`Enter` / `Space` 切换当前行。
- 键盘：`Up/Down` 在可见行内移动；`Home/End` 首尾；`Right` 展开当前（已展开则进入第一个子节点）；
  `Left` 折叠当前（已折叠则跳到父节点）。
- 折叠父节点时，若选中位于其子树内，选中上移到被折叠的节点。
- 滚动：内容高 = 可见行数 × 行高；滚轮 1.5 行；选中变化后自动保持可见。
- 焦点：`focusable()` 为真，参与 Tab 遍历；焦点环绘制在控件上下边框。

## 边界

- 不支持多选、拖拽、内联重命名、异步/懒加载子节点。
- 不支持图标、复选标记、列；节点标题为纯文本。
- 与 `Table` 不共享基类：两者都是自绘列表，但行语义与列语义差异过大，共享会引入无意义抽象。
