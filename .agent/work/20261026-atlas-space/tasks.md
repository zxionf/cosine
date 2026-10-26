# 20261026-atlas-space 任务清单

- [x] T01 `add_glyph`：每次调用重置 `last_add_out_of_space_`，空间不足时置真
- [x] T02 `ensure_glyph`：空间不足不写入 `missing_`（保持可重试）
- [x] T03 `used_ratio()` / `last_add_out_of_space()` 公开给监控与测试
- [x] T04 `test/test_font_atlas_space.cpp` + 注册
- [x] T05 回归：ctest 48/48（build + ASAN）
- [ ] T06 阶段计划勾选 M4.6 T1、SPEC 折叠、决策记录

## 完成定义

- 图集空间不足不再让字形永久不可用；缺字仍被缓存。
- `used_ratio` 可用于监控图集水位。
- 全量测试通过（含 ASAN）。
