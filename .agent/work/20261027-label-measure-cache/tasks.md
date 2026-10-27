# 20261027-label-measure-cache 任务清单

- [x] T01 `Label::append_to()`：左对齐跳过测量
- [x] T02 `Label` 测量缓存（key 含 text/size/wrap/宽度/行距/行数/图集）
- [x] T03 相关 setter 触发 `invalidate_measure()`
- [x] T04 `text_width()` 与 `measure()` 共用缓存
- [x] T05 `test/test_ui_label_measure.cpp`（6 个用例）+ 注册
- [x] T06 回归：ctest 49/49（build + ASAN）；gallery 冒烟 exit=0 / validation=0
- [ ] T07 阶段计划勾选 M4.6 T3、SPEC 折叠、决策记录

## 完成定义

- 左对齐 Label 每帧不再测量文本。
- 测量结果缓存且在所有相关变更后正确失效（含切换图集）。
