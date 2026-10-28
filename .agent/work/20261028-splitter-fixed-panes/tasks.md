# 20261028-splitter-fixed-panes 任务清单

- [x] T01 `Splitter` 支持固定一侧像素（`set_fixed_first` / `set_fixed_second`）
- [x] T02 `resolved_first()` 统一尺寸计算，`arrange` 与手柄共用
- [x] T03 `apply_drag()`：固定模式改像素、比例模式改 ratio
- [x] T04 编辑器：树固定 280、属性固定 380，预览弹性
- [x] T05 回归：ctest 56/56；冒烟 exit=0 / validation=0；导出确认
- [ ] T06 决策记录、SPEC 折叠、close

## 完成定义

- 拖树条只改树宽，拖属性条只改属性宽，两者互不影响。
- 窗口 resize 时只有预览变宽/窄。
