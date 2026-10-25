# 20261025-m3-c-debug-metrics

<!-- 为什么改、边界与不属于本次的范围 -->

## FrameStats

- `graphics::FrameStats` 统一承载 frame time、FPS 与 draw 统计。
- `collect_draw_stats(DrawList, viewport)` 统计 batches、非空 draw calls、vertices、indices、clipped batches。
- Renderer 在每帧 begin 更新 frame time/FPS，在 end 更新上一帧 DrawList 统计。
- Renderer 暴露 `frame_stats()` 只读快照；overlay 允许一帧延迟。

## DebugOverlay

- `ui::DebugOverlay` 是 Panel 子类，作为 UIRoot overlay 使用，不拦截鼠标。
- 显示 FPS、frame time、draw calls、batches、vertices、indices、clipped batches、hover/focus 节点、ScrollView offset。
- hover/focus 节点通过 dynamic_cast 映射到可读名称。
- `toggle()` 控制显示；ui_demo 用 F1 切换，`COSINE_DEBUG_OVERLAY=1` 可直接启动显示。

## 测试与边界

- `test_debug_metrics` 覆盖 DrawList 统计、FrameStats 数值和 DebugOverlay 文本。
- UI 层仍不依赖 GLFW；F1 快捷键只存在于 demo/应用层。
- 本次不做 GPU timestamp、图表、历史曲线或运行时性能预算。
