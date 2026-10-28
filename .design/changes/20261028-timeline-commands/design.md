# 20261028-timeline-commands

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.4b 第二阶段：时间轴编辑入命令栈

### 交付

| 文件 | 内容 |
|---|---|
| `src/anim/timeline.*` | `Track::find_key(time)`：按时间精确取可变指针，供命令改字段 |
| `src/anim/command.*` | `MoveKeyCommand`（拖动）、`SetKeyEasingCommand`（切插值） |
| `src/ui/timeline_view.hpp` | 上报回调：`on_move_end` / `on_remove_key` / `on_cycle_easing` |
| `src/cosine/editor.cpp` | 三个回调 → `CommandStack`，状态栏提示 |
| `test/test_ui_timeline_view.cpp` | 右键/双击改为验证「回调参数 + 数据未直接变」 |

### 设计

- **拖动**：拖动过程中 `TimelineView` 仍直接改 timeline（保证即时预览）；
  松开时对**每个真正位移过**的关键帧上报 `(track, from, to)`，编辑器逐个 push `MoveKeyCommand`。
  - push 会 apply，但此时 key 已在 `to`，`set_key_time(from, to)` 找不到 `from` → 返回 false，**幂等**；
  - `revert` 用 `set_key_time(to, from)` 正确回滚。
- **右键删除**：只上报，编辑器 push `RemoveKeyCommand`（apply 时删除、revert 时恢复）。
- **双击切插值**：只上报 `(from, to)`，编辑器 push `SetKeyEasingCommand`
  （apply 设 `to`、revert 设 `from`），因此双击后数据才真正改变。
- **多选批量拖动**：每个关键帧一条命令，撤销时逐个回滚。

### 验证

- `test_ui_timeline_view`：右键/双击断言改为「回调被调用 + track/time 参数正确 + timeline 未被直接修改」。
- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_tc.png ./cosine`：界面正常。

### 边界

- **`Splitter` 可拖分区**仍未做（固定像素分区）。
- **颜色选择**与**键盘微调**不产生撤销单元。
- 批量拖动的撤销是逐条回滚，不是「整组一次」。
- 预览时间标尺与逐帧、框选追加（Shift/Ctrl）未做。
