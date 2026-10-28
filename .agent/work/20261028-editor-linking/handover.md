# 20261028-editor-linking 交接

## 结果（M6.4b 第一阶段）

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `set_on_drag_begin` / `set_on_drag_end` |
| `src/anim/command.hpp` | `SetNodeFieldCommand`（显式 before/after + read/write 闭包） |
| `src/ui/timeline_view.hpp` | `set_selected_node()`：按节点高亮整条轨道 |
| `src/cosine/editor.*` | 命令栈、Undo/Redo 菜单、属性行接命令、树→时间轴联动 |
| `test/test_anim_command.cpp` | `test_set_node_field_command` |

## 三个要点

1. **一次拖动 = 一条命令**：`on_drag_begin` 记 `pending_before_`，`on_drag_end` 时
   `stack_.push(SetNodeFieldCommand(...))`。中间每帧只改字段不入栈，撤销栈不会被拖动帧塞满。
2. **命令不依赖属性 id**：`SetNodeFieldCommand` 用调用方的 `write(node, value)` 写字段，
   Position.x / Rotation(deg↔rad) / Scale / Size / ColorA 共用一个命令类型。
3. **抓指针要小心**：`add` 的 `slot` 是局部引用，不能在回调里捕获；先 `slot = drag.get()`
   再单独存 `ui::DragFloat* self`，回调捕获 `self`。

## 验证

- `test_set_node_field_command`：apply 改 x、y 不变；undo 回 before；redo 再改。
- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_link.png ./cosine`：界面正常，validation=0。
- `DECISIONS #137`；阶段计划 M6.4b 标「进行中（一阶段完成）」。

## 下一步（M6.4b 剩余）

1. **`Splitter` 可拖分区**：需要给 pane 设合理 min/max；`TreeView` 构造锁了固定尺寸要放宽，
   或引入一个“自由画布容器”作为中间 pane。
2. **时间轴编辑入命令栈**：`TimelineView` 目前直接改数据，需改为回调 + Editor push
   （拖动用 `MoveKeyCommand`，删除用 `RemoveKeyCommand`，插值用 `SetKeyEasingCommand`）。
3. **颜色选择与键盘微调**入栈（`ColorInput` 需要 drag/commit 回调）。
4. **预览时间标尺与逐帧**。
5. 框选追加（Shift/Ctrl）。
