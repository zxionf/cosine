# 20261027-project-serialize 任务清单

- [x] T01 `anim/project.hpp`：`Project{Scene, Timeline}`
- [x] T02 `Scene::insert` / `clear` / `set_root`（读回重建场景与 root）
- [x] T03 `Track::remove_key`、`Timeline::ensure_track` / `remove_track` / `find_track`
- [x] T04 `serialize` / `deserialize`（16 字节头、节点表、时间轴、轨道、`END!`）
- [x] T05 `deserialize` 错误路径（魔术/guard/版本/截断/长度上限/成环/多重父/未知子）
- [x] T06 `Command` + `CommandStack` + 5 个命令
- [x] T07 `test/test_anim_serialize.cpp`（8 组）
- [x] T08 `test/test_anim_command.cpp`（8 组）
- [x] T09 回归：ctest 53/53（build + ASAN）
- [ ] T10 文档：`project_format.md` 补 root 说明、决策记录、阶段计划勾 M6.1、close

## 完成定义

- 任意 `Project` 可往返序列化，且重复保存字节完全一致。
- 非法文件给出可读错误，不崩溃、不静默降级。
- 属性、节点（含子树）、关键帧的编辑都可 undo/redo。
