# 20261025-m3-5-menu-hover

<!-- 为什么改、边界与不属于本次的范围 -->

## 菜单项 hover 高亮丢失

**现象**：菜单打开后，鼠标移到菜单项上没有任何高亮，但点击能正常触发。

**根因**：

1. `update_popup_geometry()` 每次布局都会调用 `MenuPopup::set_menu_index(open_)`，
   而该函数无条件把 `hovered_` 清成 `-1`；
2. `MenuPopup::arrange()` 走 `set_pos/set_size`，每次都会 `mark_layout_dirty()`，
   于是菜单打开期间**每帧都会重新布局** → 每帧都会清掉 hover。

**修法**：

- `set_menu_index()` 只在菜单真正变化时才清空 hover；
- `MenuPopup::arrange()` 直接写 `local_/global_`，不再通过 setter 标脏；
- `update_popup_geometry()` 不再调用 `set_min_size/set_max_size/set_pos/set_size`，
  只做 `set_menu_index` + `arrange`。

## 测试

`test_ui_menu_bar` 新增两层断言：

1. **状态**：hover 后强制 `mark_layout_dirty()` 再 `ui.tick()`，`popup.hovered()` 仍为该项
   （去掉守卫会立即失败，已验证）；
2. **绘制**：把 popup append 到 `DrawList`，断言存在颜色等于 `theme.button_hover` 的四边形，
   即高亮真的进入了渲染列表。

## 边界

- 不改变 hover 颜色与命中规则；不引入 hover 过渡动画（属后续 UI 动画项）。
