# 20261025-m3-5-text-input-centering 任务清单

- [x] T01 定位：`max(pad, (height - line_height)/2)` 在行高 < 控件高度时把文字推低
- [x] T02 改为 `max(0, (height - line_height)/2)`
- [x] T03 选区高度改行高、光标在行盒内居中
- [x] T04 新增 `test_single_line_vertical_centering`（合成图集 + 中心/留白断言）
- [x] T05 反向验证：旧实现该用例失败
- [x] T06 全量回归（24/24 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成并关闭；决策 #50
