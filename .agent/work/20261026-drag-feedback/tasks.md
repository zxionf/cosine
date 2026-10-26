# 20261026-drag-feedback 任务清单

- [x] T01 Slider：拖动中手柄 accent 描边环（不依赖 FocusPolicy）
- [x] T02 Slider：拖动中数值气泡（按 step 精度格式化、居中、clamp 到控件内）
- [x] T03 Splitter：拖动中手柄 accent 填充 + 描边环
- [x] T04 Splitter：按子项 `min_size` 推导 ratio 上下限，`set_ratio` 与 `arrange` 都 clamp
- [x] T05 gallery：Slider/Splitter 标题与说明；Splitter 子面板设 min_size 演示约束
- [x] T06 `test_ui_drag_feedback`：气泡/环顶点数变化与恢复、值 clamp、ratio 约束、无 min_size 兑底
- [x] T07 全量回归：ctest 33/33 + ASAN 33/33 + gallery 冒烟 4 组（validation=0）
- [x] T08 SPEC 折叠、阶段计划勾选、决策记录

## 完成定义

- 拖动 Slider 时能看到数值气泡与手柄环，松开即消失。
- 拖动 Splitter 时手柄高亮，且不会把子项压到小于其 min_size。
- 构建零新增警告，全部测试与 ASAN 通过，gallery 可人工验收。
