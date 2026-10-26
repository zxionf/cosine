# 20261026-property-system 任务清单

- [x] T01 `anim/property.hpp`：`PropertyId` / `PropertyValue` / `PropertyKind` / `PropertyRegistry` / `prop::*`
- [x] T02 `anim/property.cpp`：注册表实现 + `to_string` + `builtin_properties()`
- [x] T03 `scene::Scene::get_property` / `set_property`（switch 分发、类型校验）
- [x] T04 `src/CMakeLists.txt`：`anim/property.cpp` 并入 `cosine_anim`
- [x] T05 `test/test_anim_property.cpp` + 注册
- [x] T06 回归：ctest 44/44（build + ASAN）
- [ ] T07 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- 十个内置属性可按 id 或名字读写，类型不匹配被拒绝且不污染字段。
- 注册表名字 ↔ id 稳定，供后续序列化与脚本使用。
- 纯逻辑可测，不依赖 UI / 渲染。
