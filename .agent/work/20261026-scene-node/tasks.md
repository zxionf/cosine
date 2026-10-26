# 20261026-scene-node 任务清单

- [x] T01 `anim/scene.hpp`：`NodeId` / `NodeType` / `Transform` / `Node` / `Scene` 声明
- [x] T02 `anim/scene.cpp`：create / find / destroy（递归）/ set_parent（防环）/ for_each
- [x] T03 `src/CMakeLists.txt`：`anim/scene.cpp` 并入 `cosine_anim`
- [x] T04 `test/test_anim_scene.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T05 回归：ctest 43/43（build + ASAN）
- [x] T06 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- 节点树可创建、移动、递归销毁，id 稳定且不复用。
- `set_parent` 拒绝成环；`for_each` 的顺序与 children 顺序一致。
- 纯数据结构，无渲染/UI 依赖，测试独立可跑。
