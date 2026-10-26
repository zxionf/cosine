# 20261026-style-transition

<!-- 为什么改、边界与不属于本次的范围 -->

## UI 动画（状态过渡）

### 目标

样式系统把控件外观收敛到 `ControlState` + `BoxStyle`，但状态切换仍是瞬时跳变：
鼠标进入的一帧内前景/背景直接换色，视觉上生硬。本次给状态切换加平滑过渡，
同时补上 UI 层的时间节拍（`tick(dt)`），为后续弹层出现/消失过渡打底。

### 时间节拍

- `UISystem::tick(float dt)` 新增；保留 `tick()` 无参重载，内部转调 `tick(1.0f / 60.0f)`，
  既有测试与 demo 的调用点不动。
- 主循环应传真实帧时间：`app.uisys.tick(frame_dt)`，并在入口把 `dt` clamp 到 `[0, 0.1]`，
  避免卡顿或断点恢复后一帧跳完整段动画。
- 更新顺序：`prune_stale_targets()` → resize/layout（脏标记）→ `update_tree(dt)`。
- `update_tree` 由 `UISystem` 统一遍历（`children()` / `overlays()`，跳过不可见项），
  容器不需要各自实现递归。
- `Widget::update(float dt)` 是新的虚钩子，默认空实现；只有需要动画的控件覆写。

### 过渡模型（集中在 Widget 基类）

```cpp
struct StyleTransition {
    StyleRole role = StyleRole::Control;
    ControlState state = ControlState::Normal;
    BoxStyle from{}, to{};
    float progress = 1.0f;   // 1 = 已完成
    bool started = false;
};
```

- 控件绘制时调用 `visual_style(role, state)`（替代直接 `theme.style(...)`）。
- 首次调用、或 `transition_ms == 0`：直接返回目标样式，不建立过渡。
- `(role, state)` 变化时：`from` 取**当前插值结果**（不是上一个状态的目标值），
  因此中途反向切换（hover→normal→hover）不会跳变；`to` 取新目标；`progress` 归零。
- `progress` 在 `Widget::update(dt)` 中按 `dt / (transition_ms/1000)` 推进并 clamp 到 1。
- 缓动固定 ease-out quad（`1 - (1-t)^2`）：起手快、收尾稳，与 UI 反馈的直觉一致。

### 只过渡颜色，几何瞬时切换

- 插值字段：`fill`、`border`、`text`、`focus_ring`。
- 不插值：`radius`、`border_width`、`text_size`。原因是圆角半径变化会改变圆弧分段数，
  相邻帧顶点数不同会让批次数与绘制结果抖动；几何瞬时切换在 100ms 尺度上不可见。
- 由此保证：同一控件在整段过渡中顶点/索引数恒定，便于测试断言。

### Theme 字段

- `float transition_ms = 120.0f`：0 表示关闭动画（立即到位）。
- `Theme::with_transition_ms(float)`：链式设置，与主题/密度/圆角/焦点策略并列。
- 过渡不随 `with_rounding(false)` 自动关闭：经典样式约束的是**静态外观**
  （直角、贴边焦点框、无内阴影），动效是独立维度，可用 Transition 选项自行关掉。
- 不做“减少动效”的系统偏好读取（无平台 API 接入），需要时把 `transition_ms` 设 0。

### 接入范围

- 已完成样式迁移的全部控件：控件内 `t.style(role, state)` 改为 `visual_style(role, state)`。
- 不接入：`Table` / `TreeView` 的行高亮（行状态是行索引，不是控件状态），
  `MenuBar` 的标题项（同理），滚动条（无状态色过渡意义）。
  它们仍用即时取色，若后续需要再单独加行级过渡。

### 验证

- `test_ui_transition`（纯逻辑，无 Vulkan）：
  - `transition_ms = 0`：状态切换后立即等于目标色。
  - 建立过渡后首帧等于 `from`；`tick(dt)` 推进到一半时各颜色分量严格位于 `from` 与 `to` 之间。
  - 累计 `dt` 达到时长后等于目标色，且 `progress` 归 1。
  - 中途反向：`progress = 0.4` 时切回 `Normal`，切换后的值接近（而非跳回）切换前的值。
  - 几何恒定：过渡全程 `radius` / `border_width` / `text_size` 等于目标；`append_to` 顶点数不变。
  - 确定性：同样的 `dt` 序列重放得到相同的插值结果。
- 现有 28 个测试继续通过（`tick()` 无参重载保证不破坏调用点）。
- gallery `Style` 分区新增 Transition 行（Off / 120 ms / 240 ms），
  环境变量 `COSINE_GALLERY_TRANSITION=off|120|240`；主循环传真实帧时间。
- gallery 冒烟：hover/pressed 切换时无 validation 错误，检查 `validation=0`。

### 边界（本次不做）

- 不做面板/弹层的出现/消失过渡（需要 alpha 合成与可见性生命周期改造，另立一条）。
- 不做尺寸/位置动画（那是 M4 时间轴的职责）。
- 不做行级高亮过渡（Table/TreeView/MenuBar 标题）。
- 不做“减少动效”系统偏好、不做可配置缓动曲线（固定 ease-out quad）。
- 不引入第三方动画库。
