# 20261025-m3-5-menu-bar

<!-- 为什么改、边界与不属于本次的范围 -->

## MenuBar

- `MenuItem`：`label` / `shortcut` / `enabled` / `action` / `separator`，
  静态构造 `MenuItem::make(...)`、`MenuItem::separator_item()`。
- `MenuBar`：横向标题栏，点击标题切换 overlay 菜单。
  - `add_menu(title, items)` / `menu_count()` / `title(i)` / `items(i)`
  - `open(i)` / `close()` / `toggle(i)` / `is_open()` / `open_menu()`
  - 键盘：`Down/Enter/Space` 打开当前菜单；打开后 `Left/Right` 切换菜单、
    `Up/Down` 移动高亮（跳过分隔线与不可用项）、`Home/End`、`Enter/Space` 激活、`Esc` 关闭。
  - 打开状态下鼠标在标题区移动会切换菜单（经典菜单栏行为）。
- overlay 组成：全屏 `MenuScrim`（点击空白关闭、把标题区 hover 转发给栏）
  + `MenuPopup`（绘制菜单项、快捷键右对齐、悬停高亮、点击激活）。

## 交互细节

- 激活顺序：先 `close()` 再执行 `action`，回调里可以安全打开对话框或改菜单。
- 关闭走 `UIRoot::remove_overlay` 延迟销毁；`close()` 幂等。
- 菜单宽度取「标题宽度」与「内容最大宽度（含快捷键）」的较大者，最小 160px。
- 分隔线高度 9px，普通行高 = `theme.control_height`。

## 边界

- 不做嵌套子菜单、勾选态/单选态、图标列、快捷键全局注册、菜单项动态刷新 API。
- 不做 macOS 全局菜单栏；移动端样式不在范围内。
