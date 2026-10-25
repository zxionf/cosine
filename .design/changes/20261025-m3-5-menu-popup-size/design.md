# 20261025-m3-5-menu-popup-size

<!-- 为什么改、边界与不属于本次的范围 -->

## 切换菜单后弹层尺寸不变

**现象**：File → Edit 悬停切换后，菜单内容变了，但弹层仍保持 File 的宽度/高度。

**根因**：`MenuPopup` 构造时把 `set_min_size/set_max_size` 固定成创建那一刻的尺寸；
`UIRoot::layout_child()` 会对每个 overlay 做 `child_size = child.clamp_size(child.measure(...))`，
`clamp_size` 把新尺寸夹回旧 min/max，于是 `arrange()` 拿到的一直是初始尺寸。

**修法**：

- 构造时不再设置 min/max，只写入 `local_ = {0, size}` 作为初始测量值；
- 尺寸完全由 `arrange()` 写入（`local_ = global_ = slot`），
  `measure()` 返回当前 `local_.size`，因此每帧测量与几何一致。

## 测试

`test_ui_menu_bar` 新增：

- File（3 行 + 分隔线 = 115px）→ 悬停切到 4 项菜单，弹层高度必须变化
- 更宽的菜单（长标签 + 长快捷键）弹层宽度 > 160px，证明没有被首次创建时的宽度锁死

## 边界

- 不引入动画过渡（尺寸变化是瞬时的）。
