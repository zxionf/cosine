# 20261025-m3-5-menu-hover handover

## 问题

菜单项 hover 无高亮（点击正常）。

## 根因链

1. `MenuPopup::arrange()` 用 `set_pos/set_size` → `mark_layout_dirty()`；
2. 因此菜单打开期间每帧都重排；
3. `update_popup_geometry()` 每帧调用 `set_menu_index(open_)`，该函数无条件清 `hovered_ = -1`；
4. hover 在下一帧被抹掉，视觉上就是「没有 hover」。

## 修法

- `set_menu_index` 加变更守卫；
- `MenuPopup::arrange` 直接写 `local_/global_`；
- `update_popup_geometry` 只做 `set_menu_index + arrange`。

## 验证

- hover 后强制布局再 tick，`hovered()` 保持；移除守卫会立刻失败（已实测）
- DrawList 中存在 `theme.button_hover` 颜色的四边形
- 24/24 ctest；ASAN 24/24；lavapipe 冒烟通过

## 经验

overlay 类控件每帧重定位时，不要在 `arrange()` 里走 `set_pos/set_size`，
否则会持续标脏 → 每帧全树重排 → 任何按帧维护的临时状态（hover/highlight）都会被重置。
