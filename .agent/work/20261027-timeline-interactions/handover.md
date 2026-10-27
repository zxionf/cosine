# 20261027-timeline-interactions 交接

## 结果（M6.3 第二阶段 + 收尾）

| 文件 | 内容 |
|---|---|
| `src/ui/timeline_view.hpp` | 吸附、右键删除、双击切插值、框选多选、批量移动 |
| `src/cosine/editor.*` | `anim::Player` 接入（Play/Pause/Reset + 预览与游标） |
| `src/cosine/main.cpp` | `COSINE_EXPORT_TIME`（导出前推进 N 秒） |
| `test/test_ui_timeline_view.cpp` | 9 组（保序/seek/拖拽/删除/切插值/吸附/scrub/框选批量/存盘往返） |

## 三个坑

1. **批量移动的顺序**：`set_key_time` 用时间精确匹配，若按选择顺序移动，
   1s→2s 会和已存在的 2s 撞车，导致第二个 key 被当成第一个移动。
   **修法**：按 delta 方向排序后再移（右移先移时间大的，左移先移小的）。
2. **`selected_time_` 要在拖动中同步**：改多选后我只更新了 `drag_origins_`，
   旧的 `test_drag_keyframe` 立刻失败（`selected_time()` 停在旧值）。
   现在每帧同步 `selected_time_` 与 `selection_` 里的时间。
3. **`sele close` 的 `replaced` 有时是预期行为**：同一 change 连续扩展时，
   design.md 的 `##` 标题与 SPEC 里已折叠的章节同名 → `replaced` 表示「用完整版覆盖」。
   只要 `design sections` 只列该一个标题，就是正常的；真正危险的是**通用标题**
   （如 `## 修复`）误匹配到无关小节。

## 验证

- `ctest` **56/56**；`test_ui_timeline_view` 9 组全过。
- `COSINE_EXPORT_PNG=/tmp/ed_box.png ./cosine`：界面正常，validation=0。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `DECISIONS #136`；阶段计划 M6.3 标「已完成」，里程碑表 ✅。

## 下一步：M6.4b 编辑器联动

1. `Splitter` 替换固定像素分区（当前比例：树 280 / 预览剩余 / 属性 380 / 时间轴 0.28）。
2. 选择联动：场景树 ↔ 属性面板 ↔ 时间轴选中行。
3. 菜单接 `CommandStack`：Undo/Redo、删除、拖动关键帧都走命令。
4. 预览逐帧与时间标尺。
5. 框选支持追加（Shift/Ctrl）。
