# 20261026-player-events 任务清单

- [x] T01 `anim/player.hpp`：`Player`（tick/seek/apply/play/pause/toggle + 事件）
- [x] T02 `anim/player.cpp`：跨过事件判定（含循环回绕）、tick 顺序（事件 → apply）
- [x] T03 `src/CMakeLists.txt`：`anim/player.cpp` 并入 `cosine_anim`
- [x] T04 `ui_anim_demo` 改用 `Player`
- [x] T05 `test/test_anim_player.cpp` + 注册
- [x] T06 回归：ctest 47/47（build + ASAN）、demo 正常退出 `validation=0`
- [ ] T07 SPEC 折叠（`--dry-run` 确认 appended）、阶段计划标记 M4 完成

## 完成定义

- 播放/暂停/跳转/事件语义明确，且 `seek` 可复现、不触发事件。
- demo 通过 Player 驱动，代码路径与库测试一致。
- 全量测试与 ASAN 通过。
