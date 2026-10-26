# 20261026-tree-view 任务清单

- [x] T01 `TreeNode` + `TreeView` 数据结构（roots、路径选中、可见行遍历）
- [x] T02 绘制：缩进、chevron、选中/hover/斑马纹、裁剪、自绘滚动条
- [x] T03 交互：点击行选中、点击箭头展开/折叠、滚轮滚动、自动保持可见
- [x] T04 键盘：Up/Down/Home/End/Left/Right/Enter/Space
- [x] T05 折叠父级时选中上移到被折叠节点
- [x] T06 `test_ui_tree`：可见行、展开折叠、点击、键盘、选中路径、滚动
- [x] T07 gallery Containers 分区接入 TreeView（文件树 + 状态回显）
- [x] T08 全量回归：构建 + ctest 26/26（ASAN 与 lavapipe 冒烟按环境）

## 完成定义

- `cmake --build build` 通过，`ctest` 全绿（含新增 tree 用例）。
- SPEC 与阶段计划更新，决策记录本次的布局与交互取舍。
