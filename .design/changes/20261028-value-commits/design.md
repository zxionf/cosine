# 20261028-value-commits

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.4b 第三阶段：颜色与键盘微调入撤销栈

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/drag_float.hpp` | `set_on_commit(before, after)`；键盘微调改走 `nudge()` 并上报 |
| `src/ui/color_input.hpp` | `set_on_commit(before, after)`；点击色块与键盘切色都上报 |
| `src/anim/command.*` | `SetNodeColorCommand`（vec4 专用；`SetNodeFieldCommand` 只处理 float） |
| `src/cosine/editor.cpp` | 属性行与颜色行接入 commit → `CommandStack` |
| 测试 | `test_keyboard_commit`（DragFloat / ColorInput 各一）+ `test_set_node_color_command` |

### 设计

- **键盘微调 = 每次按键一条命令**（不是一次拖动）。`nudge()` 只在值真的变化时
  `notify()` + `commit()`，到达边界后继续按键不会产生空命令。
- **颜色的 alpha**：色板不改 alpha，commit 时把 `before.a` / `after.a` 都对齐到节点当前 alpha。
  否则撤销会把半透明对象恢复成不透明（因为色板的 alpha 恒为 1.0）。

### 验证

- `test_keyboard_commit`（DragFloat）：右移上报 before=50/after=52；到 100 再按不上报。
- `test_keyboard_commit`（ColorInput）：切色上报 before(橙) / after(黄)。
- `test_set_node_color_command`：apply 改色、undo 回 before、redo 再改。
- `ctest` **56/56**；冒烟 exit=0 / validation=0；导出正常。

### 边界

- **`Splitter` 可拖分区**未做（当前固定像素分区）。
- 预览时间标尺与逐帧未做。
- 框选追加（Shift/Ctrl）、批量拖动合成单条命令未做。
