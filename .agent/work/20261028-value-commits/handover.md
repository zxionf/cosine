# 20261028-value-commits 交接

## 结果（M6.4b 第三阶段）

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `set_on_commit(before, after)`；键盘微调走 `nudge()` |
| `src/ui/color_input.hpp` | `set_on_commit(before, after)`；点击色块与键盘切色都上报 |
| `src/anim/command.*` | `SetNodeColorCommand`（vec4 专用） |
| `src/cosine/editor.cpp` | 属性行与颜色行接入 commit |
| 测试 | `test_keyboard_commit` × 2、`test_set_node_color_command` |

## 三个要点

1. **键盘微调 = 每次按键一条命令**：`nudge()` 只在值真的变化时 `notify + commit`，
   到边界后继续按键不会产生空命令（否则撤销栈会被无效记录塞满）。
2. **颜色不碰 alpha**：色板 alpha 恒为 1.0，若直接用它写回会把半透明对象变成不透明。
   `on_change` 保留节点 alpha，commit 时也把 `before.a/after.a` 对齐到当前 alpha。
3. **`SetNodeColorCommand` 必须单独存在**：`SetNodeFieldCommand` 只处理 float，
   `glm::vec4` 不能复用（硬塞会退化成 4 条独立命令，撤销不原子）。

## 验证

- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_vc.png ./cosine`：界面正常。
- `sele lint` 四项 ok；`DECISIONS #139`；阶段计划 M6.4b 标「三阶段完成」。

## 现在 Undo/Redo 的完整覆盖

| 操作 | 命令 |
|---|---|
| 拖动数值框 | `SetNodeFieldCommand` |
| 键盘微调数值 | `SetNodeFieldCommand`（每次按键） |
| 点色块 / 键盘切色 | `SetNodeColorCommand` |
| 拖动关键帧 | `MoveKeyCommand` |
| 右键删除关键帧 | `RemoveKeyCommand` |
| 双击切插值 | `SetKeyEasingCommand` |

## 下一步（M6.4b 剩余）

1. **`Splitter` 可拖分区**（固定像素 → 可拖；要给 pane 设合理 min/max，
   `TreeView` 构造锁死尺寸需放宽，或引入自由画布容器作为中间 pane）。
2. **预览时间标尺与逐帧**。
3. 框选追加（Shift/Ctrl）、批量拖动合成单条命令。
