# 20261025-m3-a-uicontext

<!-- 为什么改、边界与不属于本次的范围 -->

## UIContext

- `UIContext` 持有 `Theme` 与 `graphics::FontAtlas*`（非拥有），作为 UI 树的资源入口。
- `UIRoot` 拥有一个 `UIContext`；子控件通过 `Widget::context()` 沿 parent 链查找，不再直接读取全局 `theme()` / `active_font_atlas()`。
- `Widget::ui_theme()` / `Widget::font_atlas()` 优先使用 context；无 context 时保留默认 theme 和 active atlas 兼容路径，保证 standalone 测试与旧代码可迁移。
- 本次只迁移 UI 控件；`Line`/`Rectangle`/`Triangle` 的白像素 UV 仍走 active atlas，留到后续 graphics context 处理。

## 上下文传播

- `Widget::set_context()` 递归设置自身、children 与 overlays。
- `UIRoot::add/add_overlay`、`Vbox/Hbox::add`、`ScrollView::set_content` 在挂接子项时传播 context。
- `Panel` 在 context 变化时刷新 theme 的 padding/spacing；其余控件在 measure/append 时读取 `ui_theme()`。
- `Application` 在构造后把 `Renderer` 的 FontAtlas 绑定到 `UISystem` root 的 `UIContext`。
- `UISystem::context()` 暴露 root context，便于应用层配置字体/主题。

## 兼容策略

- 保留全局 `theme()` / `set_active_font_atlas()` 作为无 context 时的 fallback，不在本次删除。
- 新增 `test_ui_context` 覆盖 context 传播、字体绑定、嵌套 Panel 主题刷新。
- 现有 `test_ui_*` 与 `test_text` 不改行为，作为兼容性回归。
