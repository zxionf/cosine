# 20261027-editor-skeleton 任务清单

- [x] T01 `src/cosine/editor.hpp/.cpp`：`Editor`（菜单/工具栏/面板/排版/主题）
- [x] T02 `src/cosine/main.cpp`：Application + Editor + 主循环（含 `COSINE_EXIT_AFTER`）
- [x] T03 `src/cosine/CMakeLists.txt`：`cosine_editor` 库 + `cosine` 可执行
- [x] T04 `src/CMakeLists.txt`：`option(COSINE_BUILD_EDITOR)` + `add_subdirectory(cosine)`
- [x] T05 默认 classic 样式；圆角现代样式作为实验性选项（菜单 + 工具栏）
- [x] T06 场景树绑 `Project` 真实层级；`Add Shape` 可新增节点并刷新
- [x] T07 顺带修复 `Label::invalidate_measure` 遮蔽 `Widget` 虚函数的警告
- [x] T08 验证：`COSINE_EXIT_AFTER=3` exit=0 / validation=0；截图确认布局；ctest 53/53
- [ ] T09 文档：`docs/index.md` 结构速查与快速开始、`docs/ci.md` 冒烟命令、决策记录、close

## 完成定义

- 有可运行的产品可执行 `cosine`，三栏骨架齐全、能自适应窗口、classic 默认外观。
- `examples/` 与既有测试零回归。
