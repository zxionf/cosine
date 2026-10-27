# 20261027-property-widgets 任务清单

- [x] T01 `src/ui/drag_float.hpp`：拖动 + 键盘 + clamp/step/精度/后缀/禁用
- [x] T02 `SceneView` 2D rotation（绕节点中心、父旋转累积、轴对齐快路径保留）
- [x] T03 编辑器属性面板：11 行 + 场景树选中联动
- [x] T04 `FocusPolicy::Always`
- [x] T05 修复 `std::format` 嵌套占位符导致的数值显示错误
- [x] T06 `test/test_ui_drag_float.cpp`（6 组）+ 注册；ctest 54/54
- [x] T07 冒烟与截图：属性面板数值正确、rotation 可编辑、validation=0
- [ ] T08 色板 + 十六进制颜色输入（`ColorInput`）
- [ ] T09 gallery「属性编辑」分区（独立验收）
- [ ] T10 `SceneView` rotation 单元测试
- [ ] T11 决策记录、SPEC 折叠、阶段计划勾选

## 完成定义（本阶段）

- 编辑器可选中节点并通过拖动修改位置/旋转/缩放/尺寸/颜色，预览即时更新。
- 数值控件可键盘微调、可禁用；focus 环始终显示。
