# 20261025-m3-5-text-input-centering handover

## 问题

单行 TextInput 文字偏下（多行正常）。

## 根因

居中公式写成 `max(pad, (height - line_height)/2)`：
行高 23 < 控件高 30 时 `(30-23)/2 = 3.5`，被 `max` 提升到 `padding = 8`，文字下移约 4.5px。

## 修法

- `text_y = y + max(0, (height - line_height)/2)`
- 选区矩形高度 = `line_height`
- 光标：高度仍为字号，y 在行盒内居中

## 验证

- `test_single_line_vertical_centering`：文字中心与控件中心差 ≤ 2px；上下留白差 ≤ 4px
- 旧实现（`max(pad, ...)`）该用例失败（已实测）
- 24/24 ctest；ASAN 24/24；lavapipe 冒烟通过
- 截图：`hello` 在输入框内垂直居中

## 经验

垂直居中不要用 padding 兜底；padding 是内容缩进，与居中无关。
