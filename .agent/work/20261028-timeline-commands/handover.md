# 20261028-timeline-commands 交接

## 结果（M6.4b 第二阶段）

| 文件 | 内容 |
|---|---|
| `src/anim/timeline.*` | `Track::find_key(time)` |
| `src/anim/command.*` | `MoveKeyCommand`、`SetKeyEasingCommand` |
| `src/ui/timeline_view.hpp` | 上报回调：`on_move_end` / `on_remove_key` / `on_cycle_easing` |
| `src/cosine/editor.cpp` | 三条回调 → `CommandStack` |
| `test/test_ui_timeline_view.cpp` | 右键/双击改为验证回调参数 |

## 三个坑

1. **`sele lint` 的 `spec_process_words` 失败**：上次 close 把 design.md 的
   `### 边界（下一步）` 折叠进 SPEC，SPEC 里就留下了「下一步」。
   **修法**：SPEC 与本次 design.md 的该标题都改成 `### 边界`。
   **教训**：design.md 的**小节标题**也不要写「下一步 / 待办 / TODO」，折叠后会污染 SPEC。
2. **`MoveKeyCommand::apply` 的幂等性**：拖动时 `TimelineView` 已经改过数据，
   push 触发的 apply 会找不到 `from` → 必须容忍返回 false，否则会把 key 移动两次。
3. **时序测试断言要跟着改**：右键/双击从「改数据」改为「上报」后，
   原断言 `timeline.tracks().empty()` / `easing == EaseIn` 必然失败，要改成验证回调参数
   **并且断言数据没被直接修改**。

## 验证

- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_tc.png ./cosine`：界面正常。
- `sele lint` 四项 ok；`DECISIONS #138`；阶段计划 M6.4b 标「二阶段完成」。

## 下一步（M6.4b 剩余）

1. **`Splitter` 可拖分区**（当前固定像素；需要给 pane 设合理 min/max）。
2. **颜色选择与键盘微调入栈**（`ColorInput` 需要 commit 回调）。
3. **预览时间标尺与逐帧**。
4. 框选追加（Shift/Ctrl）、批量拖动合成单条命令。
