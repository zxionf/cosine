# 20261028-timeline-multiselect-batch

<!-- 为什么改、边界与不属于本次的范围 -->

## 时间轴多选：Shift/Ctrl 追加与整组撤销

### 背景

M6.4b 收尾。此前的框选只能「重新选一批」，无法把两次框选合并；多选拖动虽然
能一起移动，但每个关键帧各 push 一条 `MoveKeyCommand`，撤销要按 N 次。

### 修饰键从哪来

`Widget::on_mouse_down(pos, button)` 没有 `mods` 参数，而 GLFW 的鼠标按键回调
本来就带 `mods`。补一条最小通路：

```
GLFW glfwSetMouseButtonCallback(mods)
  → InputEvent.mods = map_mods(mods)
  → UISystem::dispatch_mouse_button 写入 UIContext::modifiers
  → 控件读 context()->modifiers
```

只改了三处，没有给 Widget 接口加参数，避免所有控件签名变动。

### 交互语义

| 操作 | 行为 |
|---|---|
| 普通点击关键帧 | 单选（点在已选中的关键帧上保留多选，便于直接拖） |
| **Shift / Ctrl 点击关键帧** | 切换该关键帧的选中状态，**不进入拖动** |
| 空白处框选 | 清空后重新选 |
| **Shift / Ctrl + 框选** | 保留已有选择，**追加**（按 track+time 去重） |

Shift 点击不拖动的理由：切换后再拖，「主关键帧」可能已经不在选择集里，
`primary_origin_` 与 `drag_origins_` 会失去对应关系，不如让操作分两步。

### 整组撤销

新增 `anim::CompoundCommand`：`apply` 正序、`revert` 逆序，`push` 一次只占
**一个**撤销位。`TimelineView::on_move_end` 的签名随之变为
`void(const std::vector<KeyMove>&)`，一次拖动只回调一次；编辑器按数量决定
直接 push `MoveKeyCommand` 还是包一层 `CompoundCommand`（单条不必包）。

### 顺带核实

遗留风险里记着「场景树下半部分背景色与面板底色不一致」。用取色器实测当前导出图：
树空区 `(101,105,111)`、属性面板 `(101,105,111)`，**完全一致**，该问题已不存在，
不需要再改 `TreeView`。

### 验证

- `test_anim_command` 新增 `test_compound_command`：两条 `MoveKeyCommand` 包成
  复合，push → 两处都生效；undo → 两处都回滚且只消耗一个撤销位；redo → 再生效。
- `test_ui_timeline_view` 新增两组：批量上报（2 个关键帧拖一次只回调 1 次、批量大小 2）、
  Shift/Ctrl 点击切换选择（单选 → 追加 → 移除 → Ctrl 追加）。
- `ctest` **58/58**；编辑器与 `ui_gallery` 冒烟 exit=0 / validation=0。

### 边界

- 修饰键状态在鼠标按下时读取；拖动过程中松开 Shift 不影响已开始的拖动。
- 框选不区分「从空处开始」与「从关键帧上开始」：点在关键帧上永远按点击处理。
