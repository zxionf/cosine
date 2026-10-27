# 20261027-color-input 任务清单

- [x] T01 `src/ui/color_input.hpp`：预设色板 + 高亮 + hex + 键盘循环 + 禁用
- [x] T02 编辑器属性面板：颜色 4 行 → `ColorInput` + `Color A`
- [x] T03 gallery 新增 `Properties` 分区（DragFloat × 4 + ColorInput + 状态标签）
- [x] T04 `section_names` 与导航同步为 9 项
- [x] T05 `test/test_ui_color_input.cpp`（4 组）+ 注册
- [x] T06 修正 `test_ui_gallery` 的分区数/导航数断言（8 → 9）
- [x] T07 回归：ctest 55/55；gallery SECTION=8 与 editor 冒烟 validation=0
- [ ] T08 `docs/examples.md` 分区表补 Properties、索引范围改 0–8
- [ ] T09 决策记录、SPEC 折叠、阶段计划勾选、close

## 完成定义

- gallery 能单独验收 DragFloat 与 ColorInput。
- 编辑器可用色板改节点颜色，且不破坏 alpha。
