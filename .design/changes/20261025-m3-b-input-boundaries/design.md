# 20261025-m3-b-input-boundaries

<!-- 为什么改、边界与不属于本次的范围 -->

## KeyCode 与 Modifier

- UI 层不直接依赖 GLFW 键码；`events/key_code.hpp` 定义稳定的 `KeyCode` 与 `Modifier` 位标志。
- `InputEvent` 的 `key/mods` 改为 `KeyCode/Modifier`；GLFW 键码到 `KeyCode` 的映射只存在于应用层 `xel_application.cpp`。
- 首版只覆盖 UI 需要的键：Tab、Enter/KeypadEnter、Space、Backspace、Delete、方向键、Home/End、A。
- `Modifier` 用位标志支持 Shift/Ctrl/Alt/Super 组合。

## 头文件边界

- `ui/` 公共头不再包含 `<GLFW/glfw3.h>` 或 `backend/window.hpp`。
- `UISystem` 对 `backend::Window` 使用前向声明，窗口实现细节留在 `ui_system.cpp`。
- Button/Checkbox/Slider/TextInput/Widget 只依赖 `events/key_code.hpp`。
- 删除残留未使用的 `ui/tester.hpp`。

## 兼容与迁移

- 测试中的 `GLFW_KEY_*` / `GLFW_MOD_*` 改为 `KeyCode` / `Modifier`。
- 保留 GLFW 映射集中在应用层，后续 Windows/其他后端可替换。
- 本次不改图形 primitives 的 active atlas 依赖，也不引入 IME；这些留到后续。
