# 20261027-tooltip-dpi-fix 交接

## 结果

`examples/ui_gallery/main.cpp`：

```cpp
// 之前
const glm::vec2 cursor = cursor_to_framebuffer(app.window_);
// 之后
const glm::vec2 cursor = cursor_to_framebuffer(app.window_) / app.ui_scale();
```

T8 把 UI 坐标系改成逻辑像素后，gallery 仍把 framebuffer 像素当逻辑坐标交给
`gallery.update_tooltip()` / `Tooltip::update_hover()`，于是偏移量 = `ui_scale` 倍。

## 验证

- `COSINE_UI_SCALE=2 ./ui_gallery`：exit=0、validation=0。
- `ctest` 51/51（build + ASAN）。
- `sele close` 为 `appended`（`DECISIONS #126`）。

## 用户验证

```bash
cd build/examples
COSINE_UI_SCALE=2 ./ui_gallery
# 悬停右侧 Overview 里的 "Hover this line to see an overlay tooltip."
# tooltip 应紧贴鼠标；把倍率改成 3 也应贴住
```

## 重要教训：sele close 的标题模糊匹配

本次 design 一开始用了 `## 修复` 作为二级标题，`sele close --dry-run` 显示：

```
修复 -> replaced (spec section: 滚动目标保持（M3.6 修复）)
```

即**会覆盖 SPEC 的 M3.6 小节**。已通过把 design 的结构改成
`## <change 专属标题>` + `### 现象/根因/改动/验证/边界` 规避，重新 dry-run 为 `appended`。

**结论**：design.md 的二级标题必须是本次变更专属的名字，
不要用「修复 / 现象 / 验证」这类通用词；通用词留在三级标题。

## 下一步

- 建议把「光标 → 逻辑坐标」收敛为 `Application::cursor_position()`，避免各 example 各写一份。
- M4.6 剩余：T5 字距 / T9 TextSpan / T10 字体覆盖 / T11 IME / T12 SDF / T13 TreeView；或 M5。
