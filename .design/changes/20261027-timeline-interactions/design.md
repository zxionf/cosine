# 20261027-timeline-interactions

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.3 第二阶段：时间轴交互与 Player 联动

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/timeline_view.hpp` | 吸附、右键删除、双击切插值、`on_changed` 回调 |
| `src/cosine/editor.*` | 接入 `anim::Player`：Play/Pause/Reset、tick 驱动预览与游标 |
| `src/cosine/main.cpp` | `COSINE_EXPORT_TIME`（导出前推进 N 秒，用于验证播放） |
| `test/test_ui_timeline_view.cpp` | 新增 3 组（删除、切插值、吸附） |

### 交互规则

- **拖动吸附**：吸附到整秒或其它关键帧，阈值 6px 对应的时间。
- **右键**：删除命中的关键帧；轨道变空则一并 `remove_track`。
- **双击**：插值循环 `Linear → EaseIn → EaseOut → EaseInOut → CubicBezier`，用 `remove_key` + `add_key` 实现（`keys()` 只读）。
- **点标尺**：`player_->seek()`，游标同步。

### Player 联动

`Editor` 持有 `std::unique_ptr<anim::Player>`（只持 scene/timeline 引用），
`tick()` 里 `player_->tick(dt)` → `timeline_view_->set_time(player_->time())`；
`Play/Pause` 切换并改按钮文字，`Reset` 归零。

### 验证

- `ctest` **56/56**；`test_ui_timeline_view` 共 7 组（含保序、seek、拖拽、删除、切插值、吸附）。
- `COSINE_EXPORT_PNG=/tmp/ed_t1.png COSINE_EXPORT_TIME=1.2 ./cosine`：
  Card 从 `(120,150)` 移到约 `(390,150)`，播放头落在 1.2s，按钮显示 `Pause`，validation=0。

### 框选多选与批量移动（补充）

- 轨道区空白按下 → 框选（标尺区仍是 scrub）；选框半透明 + accent 边框。
- 选中集合用 `track + time` 表示；点击未选中的关键帧重置为单选，点击已选中的保留多选。
- 拖任一选中关键帧 → 全部按同一 delta 平移。
- **防碰撞**：按 delta 方向排序后再移（右移先移时间大的），否则 1s→2s 会撞上已存在的 2s。
- 拖动中同步 `selected_time_` 与 `selection_` 的时间，否则高亮与既有测试都会失效。

### 存盘往返验收

`test_edit_then_round_trip`：拖关键帧 → `serialize` → `deserialize` → 轨道数与关键帧时间一致。

### 边界（下一阶段）

- **框选多选与批量移动**未做。
- 时间轴编辑**未接 `CommandStack`**（拖动/删除/切插值都直接改 timeline，撤销留到 M6.4b 统一接入）。
- 属性面板不随播放实时刷新（只在选中变化时同步）——播放时应显示被轨道驱动的值，留到 M6.4b。
