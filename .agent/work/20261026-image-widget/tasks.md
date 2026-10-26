# 20261026-image-widget 任务清单

- [x] T01 `Image` 控件：`image.hpp`（构造、`source` / `texture_id` / `fit` / `tint` / 占位文案）
- [x] T02 占位绘制：圆角底色 + 边框 + 对角交叉 + 居中文案（裁剪在控件内）
- [x] T03 `measure` / `arrange`，兼容 `size_percent` / `aspect_ratio`
- [x] T04 `test/test_ui_image.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T05 gallery `Layout` 分区：两张占位图（有来源 / 空来源）
- [x] T06 全量回归：ctest 40/40 + ASAN 40/40 + gallery 冒烟 3 组（validation=0）
- [x] T07 M3.5 归档：阶段计划勾选、STATE 更新、SPEC 折叠与决策记录

## 完成定义

- `Image` 可在任意容器中占位，尺寸支持百分比与宽高比，来源/纹理句柄/fit 接口稳定。
- 未接纹理时视觉上明确是占位（交叉线 + 文件名），不会误导。
- 构建零新增警告，全部测试与 ASAN 通过。
