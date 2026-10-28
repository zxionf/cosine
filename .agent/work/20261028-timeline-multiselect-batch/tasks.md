# 20261028-timeline-multiselect-batch 任务清单

- [x] T01 修饰键通路：GLFW mods → `InputEvent.mods` → `UIContext::modifiers`
- [x] T02 Shift/Ctrl 点击关键帧切换选中（不进入拖动）
- [x] T03 Shift/Ctrl 框选追加（去重）
- [x] T04 `anim::CompoundCommand`（正序 apply / 逆序 revert）
- [x] T05 `on_move_end` 改批量上报，编辑器按需包复合命令
- [x] T06 `test_compound_command` + 时间轴两条新测试，ctest 58/58
- [x] T07 核实场景树背景色已与面板一致（取色实测）
- [x] T08 编辑器 / gallery 冒烟 exit=0 / validation=0
- [ ] T09 决策记录、SPEC 折叠、close

## 完成定义

- Shift/Ctrl 能把两次选择合并，也能单独取消某个关键帧。
- 多选拖动一次，撤销一次即全部回滚。
