# 20261025-m3-5-text-input-multiline 任务清单

## 多行 TextInput

- [x] T01 行模型：line_of / line_start / line_end / nearest_index
- [x] T02 `set_multiline` / `set_preferred_size` / `set_max_height`；多行高度按行数测量
- [x] T03 渲染：逐行绘制、跨行选区、光标、纵向滚动
- [x] T04 交互：Enter 换行、Up/Down 跨行、Home/End 行内、点击定位
- [x] T05 新增 `test_ui_text_input`（单行回归 + 多行导航/选区/换行/高度上限）
- [x] T06 gallery Inputs 分区展示多行输入（行数/字符数回显）
- [x] T07 全量回归（20/20 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 多行 TextInput 完成；20/20 ctest、ASAN、lavapipe 冒烟通过
- 修复 line_start 实现错误（导致 Up/Down 落到下一行行首）
