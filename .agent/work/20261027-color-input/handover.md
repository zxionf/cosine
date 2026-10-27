# 20261027-color-input 交接

## 结果（M6.2 第二阶段）

| 文件 | 内容 |
|---|---|
| `src/ui/color_input.hpp` | `ColorInput`：预设色板（6 色）+ 当前色环 + hex 只读 + 键盘循环 + 禁用 |
| `examples/ui_gallery/ui_gallery.hpp` | 第 9 分区 `Properties`（index 8）：DragFloat × 4 + ColorInput + 状态标签 |
| `src/cosine/editor.*` | 颜色 4 行 → 1 行 `ColorInput` + 独立 `Color A` |
| `test/test_ui_color_input.cpp` | 4 组用例 |

## 两个坑

1. **gallery 分区数 8 → 9**：`test/test_ui_gallery.cpp` 有两处硬编码断言
   （`sections.size() == 8`、`nav_buttons.size() == 8`），加分区后必须同步，否则测试挂。
2. **`sele close` 会重写 `STATE.md`**：同一批操作里先 `edit STATE.md` 再 `sele close`，
   close 会用旧值覆盖 Phase/Blocked/Next。**正确顺序：先 close，再改 STATE.md**。

## 验证

- `test_ui_color_input`：clamp、键盘循环（含首尾回绕）、禁用、自定义色板。
- `ctest` **55/55**。
- `COSINE_GALLERY_SECTION=8 COSINE_EXIT_AFTER=3 ./ui_gallery`：exit=0、validation=0。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0；属性面板颜色行显示为色块条 + hex。
- `DECISIONS #133`；阶段计划 M6.2 标「已完成」，里程碑表补一行。

## 已知边界

- hex 是**只读显示**，不能直接输入（需要 TextInput 组合）。
- 色板固定 6 色（可 `set_palette` 换），不做 HSV 取色器。
- `SceneView` rotation 仍无单元测试（靠编辑器截图验收）。

## 下一步：M6.3 时间轴编辑视图

- 标尺 + 轨道行 + 关键帧菱形；时间↔像素映射与缩放。
- 单键拖拽、框选多选、吸附、右键删除/切换插值。
- 与 `Player` 联动：拖游标即 seek，播放时游标跟随。
- 数据侧已齐：`Timeline`/`Track`/`Keyframe` + `CommandStack` 的 AddKey/RemoveKey 命令。
