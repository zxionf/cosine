# 20261028-splitter-handle-hit 任务清单

- [x] T01 `handle_rect()` 改用 `resolved_first()`
- [x] T02 新增 `test/test_ui_splitter.cpp`（4 组，含「中间不命中」回归断言）
- [x] T03 注册测试 + ctest 57/57
- [x] T04 冒烟 exit=0 / validation=0；导出正常
- [ ] T05 决策记录、SPEC 折叠、close

## 完成定义

- 三条分割条都能拖；命中区与绘制位置严格一致。
