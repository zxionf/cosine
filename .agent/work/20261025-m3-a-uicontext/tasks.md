# 20261025-m3-a-uicontext 任务清单

## M3-A UIContext

- [x] A01 新增 `UIContext`（Theme + FontAtlas*）
- [x] A02 Widget 增加 context 指针、`context()`、`ui_theme()`、`font_atlas()`、递归 `set_context()`
- [x] A03 UIRoot 持有 context；UISystem 暴露 context；Application 绑定 Renderer FontAtlas
- [x] A04 UI 控件改用 `ui_theme()` / `font_atlas()`，Panel 响应 context padding/spacing
- [x] A05 容器 add/set_content 传播 context
- [x] A06 新增 `test_ui_context`，覆盖传播/字体/主题
- [x] A07 全量构建、ctest、ui_demo validation 冒烟

## 当前进度

- UIContext 已落地：Widget context 传播、ui_theme()/font_atlas()、Application 绑定 Renderer 字体
- 10/10 ctest 通过；ui_demo 运行正常
