# 20261027-line-height-metrics 任务清单

- [x] T01 `FontAtlas::normalize_metrics`（line_height ≥ ascent - descent，且 ≥ 1.0）
- [x] T02 `begin_generated` 存入前归一化
- [x] T03 `FontFace::metrics()` 改用归一化（去掉重复 clamp）
- [x] T04 `test/test_font_atlas_metrics.cpp`（4 个用例）+ 注册
- [x] T05 回归：ctest 50/50（build + ASAN）；ui_demo 校验错误 0
- [x] T06 ui_demo 目视：96px “Hqyp gjpqy” 行框完整包住降部
- [ ] T07 阶段计划勾选 M4.6 T4、SPEC 折叠、决策记录

## 完成定义

- 行框与控件高度不再小于 `ascent + |descent|`。
- 多行测量高度与 `line_height` 一致。
