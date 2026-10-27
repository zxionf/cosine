# 20261027-tooltip-dpi-fix

<!-- 为什么改、边界与不属于本次的范围 -->

## tooltip 在非 1 倍 UI 缩放下位置偏移

### 现象

`COSINE_UI_SCALE != 1` 时，gallery 的 overlay tooltip 不跟随鼠标：位置整体偏向右下，
倍率越大偏得越多（2 倍时可能整个移出窗口）。

### 根因

T8 把 UI 坐标系改成逻辑像素（`root` 尺寸 = `extent / ui_scale`），输入回调也随之换算。
但 `examples/ui_gallery/main.cpp` 给 tooltip 的光标仍是 **framebuffer 像素**：

```cpp
const glm::vec2 cursor = cursor_to_framebuffer(app.window_);  // framebuffer 像素
gallery.update_tooltip(cursor, dt);                            // 被当作逻辑坐标
```

`Tooltip::update_hover()` 的契约是逻辑坐标（与 `root->global_bounds()` 同系），
于是偏移量正好等于 `ui_scale` 倍。

### 改动

```cpp
const glm::vec2 cursor = cursor_to_framebuffer(app.window_) / app.ui_scale();
```

仅改 gallery 一处；`Tooltip` 自身逻辑未动。

### 验证

- `COSINE_UI_SCALE=2 ./ui_gallery`：exit=0、validation=0。
- `ctest` **51/51**（build 与 ASAN）。
- `Tooltip::update_hover()` 的翻边与夹取已有 `test_ui_tooltip.cpp` 覆盖。

### 遗留改进（未做）

各 example 各自写「窗口坐标 → framebuffer」的转换函数（gallery 与 `xel_application.cpp` 各一份）。
建议后续收敛为 `Application::cursor_position()`（返回逻辑坐标），避免再出现同类漂移。
