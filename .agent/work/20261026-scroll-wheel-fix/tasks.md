# 20261026-scroll-wheel-fix 任务清单

- [x] T01 `Table::arrange`：`scroll_target_ = clamp_scroll(scroll_target_)`，保留平滑目标
- [x] T02 `TreeView::arrange`：同上
- [x] T03 复现验证：`on_scroll` → `arrange` → `update` 后 target 保持、offset 推进
- [x] T04 `UiGallery::set_scroll_mode` + Style 分区 Smooth / Line 按钮与状态标签
- [x] T05 `COSINE_GALLERY_SCROLL_MODE` 走同一入口
- [x] T06 回归：ctest 41/41（build + ASAN）、冒烟三组 `validation=0`、截图确认
- [x] T07 决策记录与归档

## 完成定义

- Table / TreeView 滚轮平滑滚动可用，Line 模式仍立即到位。
- gallery 顶部/风格页可用按钮切换两种模式，状态标签同步更新。
- 全量测试与 ASAN 通过，冒烟 `validation=0`。
