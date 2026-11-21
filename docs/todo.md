# TODO

## Scrim / 模态键盘与焦点隔离不完整

- 记录时间：2026-11-19T23:53:04+08:00
- 分支：`main`
- 提交：`7ac90ba31b3681d62983c72c3e315cf14ec2a420`
- 状态：已解决（UI-19）

### 问题

`src/ui/core/scrim.hpp` 的遮罩层本身已实现：

- 全屏半透明绘制（`append_scrim`）
- `fills_parent()` 由 `UIRoot::layout_child` 拉成全屏
- `hit_testable()` 阻止指针穿透
- `on_mouse_down` / `on_mouse_up` / `on_scroll` 吞噬事件
- 不参与 Tab 焦点
- `UIRoot::push_modal()` / `pop_modal()` 维护 Scrim 与模态窗口的 z-order / 嵌套栈

未完成的是模态的键盘与焦点隔离：

1. `UISystem::dispatch_key()` / `dispatch_char()` 只把事件发给 `focused_`，没有检查 `root_->modal_top()`。打开模态时如果焦点仍在下层控件，键盘会穿透到被遮罩的控件。
2. `UIRoot::push_modal()` 不自动把焦点移到模态窗口内的第一个可聚焦控件；`focus_next()` 只在用户按 Tab 时才用 `modal_top()` 作为作用域。
3. `pop_modal()` 不记录/恢复打开模态前的焦点；关闭后焦点可能留在已销毁窗口，依赖 `prune_stale_targets()` 清理。
4. `Scrim` 没有 `on_click` / dismiss 回调，点击遮罩不会关闭模态窗口。
5. `Scrim` 只处理 down/up/scroll；右键、双击、长按没有显式行为。
6. 菜单与下拉分别使用 `MenuScrim` / `DropdownScrim`，不复用 `Scrim`，说明遮罩能力目前只覆盖“窗口模态”。

### 影响面

- 文件：
  - `src/ui/core/scrim.hpp`
  - `src/ui/root.cpp`
  - `src/ui/ui_system.cpp`
  - `src/ui/composites/dialog.hpp`
  - `src/ui/composites/menu_bar.hpp`
  - `src/ui/composites/dropdown.hpp`
- 功能：
  - 模态窗口打开期间的键盘/字符输入隔离
  - 模态打开/关闭时的焦点转移与恢复
  - 点击遮罩关闭模态（如需要）
  - 右键/双击/长按在遮罩上的行为
- 用户场景：
  - 打开/保存文件对话框、确认对话框等窗口模态
  - 菜单、下拉的遮罩不受影响（使用各自的 scrim 实现）
- 测试：
  - 目前缺少“模态打开时键盘不穿透”“关闭后焦点恢复”的测试覆盖

### 建议修复

1. `dispatch_key` / `dispatch_char` 限制在 `modal_top()` 子树内；若焦点不在模态子树，忽略或先转移焦点。
2. `push_modal()` 时记录当前焦点，并聚焦模态内第一个可聚焦控件。
3. `pop_modal()` 恢复记录的焦点（若仍有效）。
4. 如需点击遮罩关闭：给 `Scrim` 增加 dismiss 回调，并只对顶层模态生效。
5. 明确右键/双击/长按在模态遮罩上的语义（吞掉或触发关闭）。
6. 补测试：模态键盘隔离、焦点恢复、嵌套模态。

> 已解决（UI-19）：`dispatch_key`/`dispatch_char` 限定在最上层模态子树；`push_modal` 快照焦点并聚焦
> `preferred_focus()` → 首个可聚焦 → 窗口自身；`pop_modal` 恢复快照；`Scrim` 显式吞掉 click/right_click/
> double_click/long_press 并支持可选 `set_on_dismiss`（默认不关 Window 模态）；`DialogHost` 经
> `push_modal(host, with_scrim=false)` 纳入模态作用域；新增 `test/test_ui_modal.cpp`。详见 `docs/ui_architecture.md` §5。

## Layer 未完全实现

- 记录时间：2026-11-20T00:14:10+08:00
- 分支：`main`
- 提交：`7ac90ba31b3681d62983c72c3e315cf14ec2a420`
- 状态：已解决（方案 A：完全实现 Layer 排序，见 UI-20）

### 现状

`src/ui/widget.hpp:138` 定义了：

```cpp
typedef enum class Layer
{
    Content,
    Floating,
    Modal,
};
```

`layer()` 默认返回 `Content`。

全仓库唯一的重写是：

```cpp
// src/ui/core/scrim.hpp:30
Layer layer() const override { return Layer::Floating; }
```

但没有任何代码读取 `layer()`：

- 不参与绘制排序；
- 不参与命中测试；
- 不参与布局；
- 不参与模态/overlay 管理。

所以 `Scrim::layer()` 当前没有实际效果，`Layer` 是预留但未接线的语义。

实际分层由另一套机制完成：

1. `children_` 与 `overlays_`（`UIRoot::add_overlay` / `overlay_paint_order`）
2. `z_index`（同层内排序）
3. `modals_` / `modal_scrims_`（窗口模态栈，Scrim z = window.z - 1）
4. `MenuScrim` / `DropdownScrim`（菜单与下拉各自的遮罩）

### 影响面

- 文件：
  - `src/ui/widget.hpp`
  - `src/ui/core/scrim.hpp`
  - `src/ui/root.cpp`
  - `src/ui/ui_system.cpp`
  - `src/ui/composites/window.hpp`
  - `src/ui/composites/dialog.hpp`
  - `src/ui/composites/menu_bar.hpp`
  - `src/ui/composites/dropdown.hpp`
- 功能：
  - `Layer` 是死代码，重写 `layer()` 不会改变绘制/命中/模态行为；
  - 容易让后续开发者误以为改 `layer()` 就能调整层级；
  - 与 overlay / z-index / 模态栈三套机制并存，语义重叠。
- 测试：
  - 没有针对 Layer 绘制顺序、命中优先级、跨层遮挡的测试。

### 建议

方案 A：补全 Layer

1. 在 `UIRoot` / `UISystem` 的绘制与命中排序中引入 Layer：`Content` → `Floating` → `Modal`。
2. 把 overlay / modal 映射到 Layer，而不是维持两套平行机制。
3. 让 `Window`、`Dialog`、`Scrim`、`Menu`、`Dropdown` 明确返回对应 Layer。
4. 明确 `z_index` 是 Layer 内排序还是跨 Layer 覆盖。
5. 补测试：不同 Layer 的绘制顺序、命中优先级、模态遮挡。

方案 B：删除 Layer

- 删除 `Layer` 枚举与 `layer()`，以及 `Scrim` 的重写；
- 统一使用 overlay / z-index / 模态栈，避免误导。

> 已解决（UI-20）：Layer 现在是跨 `children_`/`overlays_` 的唯一主排序键（`Widget::layer_paint_order()`），
> 绘制/命中/Tab 三处消费；`z_index` 仅层内生效；各控件归属已落地（Scrim→Modal、模态 Window→Modal、
> DialogHost/Menu/Dropdown/Tooltip→Floating）；新增 `test/test_ui_layer.cpp`。详见 `docs/ui_architecture.md` §3。
