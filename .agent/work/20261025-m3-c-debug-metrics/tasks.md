# 20261025-m3-c-debug-metrics 任务清单

## M3-C 调试指标

- [x] C01 新增 `graphics::FrameStats` / `DrawStats` 与 `collect_draw_stats`
- [x] C02 Renderer 统计 frame time/FPS/draw calls/vertices/indices/clipped
- [x] C03 新增 `ui::DebugOverlay`，显示指标与 hover/focus/scroll
- [x] C04 ui_demo 接入 overlay，F1 切换，`COSINE_DEBUG_OVERLAY` 直接显示
- [x] C05 新增 `test_debug_metrics`；更新 ui_demo 集成测试
- [x] C06 全量构建、ctest、ui_demo validation 冒烟

## 当前进度

- FrameStats/DebugOverlay 已落地；12/12 ctest 通过；ui_demo 截图确认指标显示
