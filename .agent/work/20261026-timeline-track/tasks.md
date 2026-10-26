# 20261026-timeline-track 任务清单

- [x] T01 `anim/timeline.hpp`：`Keyframe` / `Track` / `Timeline` 声明
- [x] T02 `anim/timeline.cpp`：`lerp_property`（含阶跃类型）、`Track::add_key` / `sample`
- [x] T03 `Timeline`：duration / fps / loop / time_scale / play / pause / seek / advance / apply
- [x] T04 `src/CMakeLists.txt`：`anim/timeline.cpp` 并入 `cosine_anim`
- [x] T05 `test/test_anim_timeline.cpp` + 注册
- [x] T06 回归：ctest 45/45（build + ASAN）
- [ ] T07 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- 轨道按时间采样，两端外推、单帧恒定、缓动生效。
- 循环 / 非循环、`time_scale`、确定性 seek 均符合预期。
- `apply` 写回 Scene 且同一时间可复现。
