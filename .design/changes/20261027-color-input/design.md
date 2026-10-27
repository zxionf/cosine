# 20261027-color-input

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.2 第二阶段：ColorInput 与 gallery 属性编辑分区

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/color_input.hpp` | `ColorInput`：预设色板 + 当前色高亮 + 十六进制显示 + 键盘循环 |
| `examples/ui_gallery/ui_gallery.hpp` | 新增第 9 个分区 `Properties`（index 8）：DragFloat × 4 + ColorInput |
| `src/cosine/editor.*` | 颜色 4 行 → 1 行 `ColorInput` + 独立 `Color A` 行 |
| `test/test_ui_color_input.cpp` | 4 组用例 |

### 设计

- `ColorInput` 一行内放 label + N 个预设色块 + 右侧 `#RRGGBB`；空间不足时不画 hex（避免与色块挤）。
- 点击色块即选中；键盘 Left/Right 在预设间循环（含首尾回绕）。
- **不回写 alpha**：回调只取 RGB，alpha 保留节点原值（否则选色会把半透明对象变不透明）。
- 默认色板 6 色（橙/黄/绿/蓝/紫/白），可 `set_palette` 替换。

### 踩到的坑

**gallery 分区数从 8 变 9**，`test/test_ui_gallery.cpp` 里有两处硬编码断言需要同步：
`gallery.sections.size() == 8` 与 `gallery.nav_buttons.size() == 8`。

### 验证

- `test_ui_color_input`：clamp、键盘循环（含回绕）、禁用、自定义色板。
- `ctest` **55/55**。
- `COSINE_GALLERY_SECTION=8 COSINE_EXIT_AFTER=3 ./ui_gallery`：exit=0、validation=0。
- 编辑器截图：属性面板颜色行显示为色块条 + hex，其余 8 行数值正常。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。

### 边界（本次未做）

- 十六进制**输入**（只读显示）：需要与 `TextInput` 组合，随属性面板二次打磨再做。
- 颜色选择器（HSV 轮盘/取色器）不做，预设色板已覆盖常用取值。
- `SceneView` rotation 的单元测试仍未补（靠编辑器截图人工验收）。
