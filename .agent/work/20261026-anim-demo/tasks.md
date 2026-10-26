# 20261026-anim-demo 任务清单

- [x] T01 `examples/ui_anim_demo`：5 条缓动轨道 + Pause/Play/Reset + 进度与时间标签
- [x] T02 `examples/CMakeLists.txt`：`ui_anim_demo` target（链接 `cosine_app` + `cosine_anim`）
- [x] T03 修复绝对定位漂移：`Widget::absolute_pos_` + `set_absolute_pos`，四个容器改用该偏移
- [x] T04 `ColorRect::set_color`
- [x] T05 `test_ui_absolute_zindex` 新增「多次 arrange 不漂移」用例
- [x] T06 回归：ctest 43/43（build + ASAN）、两个 example 冒烟 `validation=0`、截图确认
- [ ] T07 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- example 可播放/暂停/重置，五条曲线在同屏直观对比。
- 绝对定位子项在多帧布局后不漂移。
- 全量测试与两个 example 冒烟通过。
