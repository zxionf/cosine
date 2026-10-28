# 20261028-editor-linking

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.4b 第一阶段：选择联动与属性撤销

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `set_on_drag_begin` / `set_on_drag_end`（把一次拖动合成一个撤销单元） |
| `src/anim/command.hpp` | `SetNodeFieldCommand`：显式 before/after + writer，不依赖属性 id |
| `src/ui/timeline_view.hpp` | `set_selected_node()`：场景树选中的节点对应轨道行高亮 |
| `src/cosine/editor.*` | 命令栈、Undo/Redo 菜单、属性行接入、树→时间轴联动 |
| `test/test_anim_command.cpp` | 新增 `test_set_node_field_command` |

### 关键设计

1. **拖动 → 一条命令**：`DragFloat` 在按下时触发 `on_drag_begin`（编辑器记 `pending_before_`），
   松开时触发 `on_drag_end`（`stack_.push(SetNodeFieldCommand(before, self->value()))`）。
   中间每帧只改节点字段、不入栈，避免撤销栈被拖动帧塞满。
2. **`SetNodeFieldCommand` 不关心属性 id**：用调用方给的 `writer(node, value)` 写字段，
   于是 Position.x、Rotation(deg↔rad)、Scale、Size、Color A 共用同一个命令类型。
   读值也由 `read(node)` 提供，用于拖动开始时抓 before。
3. **选择联动**：`on_scene_selected` 把 `selected_` 传给 `TimelineView::set_selected_node`，
   时间轴按 `track.node() == selected_node` 高亮整行，和场景树形成对照。

### 验证

- `test_set_node_field_command`：apply 改 x、y 不变；undo 回 before；redo 再改。
- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_link.png ./cosine`：界面正常，validation=0。

### 边界（下一步）

- **`Splitter` 可拖分区**未做（当前仍固定像素分区）。
- **时间轴编辑未入命令栈**（拖动/删除/切插值仍直接改数据）。
- **颜色选择**与**键盘微调**不产生撤销单元（只有拖动数值框会）。
- 预览时间标尺与逐帧未做。
- 框选追加（Shift/Ctrl）未做。
