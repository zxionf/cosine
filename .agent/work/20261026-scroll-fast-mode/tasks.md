# 20261026-scroll-fast-mode 任务清单

- [x] T01 `ScrollMode` 增加 `Fast`
- [x] T02 `ScrollView`：`idle_` + `on_update` 分模式（tau/snap/idle 加速）
- [x] T03 `TreeView` / `Table`：同样支持 `Fast`
- [x] T04 gallery Style 分区增加 `Fast` 按钮
- [x] T05 `test_ui_smooth_scroll` 新增 Fast 用例
- [x] T06 回归：ctest 41/41、ASAN、冒烟与截图
- [x] T07 决策记录与归档

## 完成定义

- 三种模式（Smooth / Fast / Line）在任何滚动容器上可切换。
- Fast 在停止输入后明显更快收尾，且与帧率无关。
- 全量测试与 ASAN 通过。
