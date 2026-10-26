# 20261026-scroll-wheel-fix 交接

## 结果

- `Table::arrange` / `TreeView::arrange`：`scroll_target_ = clamp_scroll(scroll_target_)`
  （原来写的是 `= scroll_`，每帧把滚轮目标清零）。
- `UiGallery::set_scroll_mode(ScrollMode)`：统一切换分区 ScrollPanel、示例 ScrollView、
  Table、TreeView。
- Style 分区新增 **Smooth / Line** 按钮 + `scroll: smooth|line` 状态标签；
  `COSINE_GALLERY_SCROLL_MODE` 走同一入口。

## 验证

- 复现：`on_scroll` → `arrange` → `update`，target 保持 126、offset 推进 29.5。
- `ctest` 41/41（build 与 ASAN）。
- 冒烟：Style、Containers、Containers(line) 三组 `validation=0`。
- 截图：Style 分区可见 Smooth / Line 按钮与 `scroll: smooth`。

## 注意事项

### sele close 的 `replaced` 会覆盖 SPEC 小节

`sele close` 输出 `folded: Table / TreeView 无法滚动… -> replaced` 时，
它把 **`## TreeView` 小节**的原有内容整段替换掉了（标题模糊匹配）。
本次已用 `git diff .design/SPEC.md` 找回原文并恢复，新内容改放到
`## 滚动目标保持（M3.6 修复）` 独立小节。

**以后 close 前先 `--dry-run`，看到 `replaced` 要人工确认目标小节是否正确。**

### 平滑滚动的两个易踩点

1. `arrange` 里不能把 `target` 重置为当前位置（会每帧清零滚轮目标）。
2. 平滑推进放在 `on_update(dt)`，`arrange` 只做 clamp。

## 后续

- M4 按 easing → scene-node → property-system 启动。
