# 20261026-pointer-events

<!-- 为什么改、边界与不属于本次的范围 -->

## 指针事件

### 现状

- `event::InputEvent` 已带 `mouse_button`（直接沿用 GLFW 值：0 左 / 1 右 / 2 中）。
- `UISystem::dispatch_mouse_button` 对**任何**按钮都做同一套事：可聚焦就抢焦点、走 `on_mouse_down/up`、同目标再走 `on_click`。
- 因此右键也会抢焦点、也会触发 `on_click`（按钮被右键激活）；双击与长按完全没有，控件无法表达「双击重命名」「右键菜单」「长按拖拽」。

### 目标

补齐右键、双击、长按三种语义，作为右键菜单、TreeView 内联重命名、长按拖拽的前置能力；
所有判定用 UI 自己的时间基准，保证纯逻辑可测。

### 设计

**新增虚钩子（`Widget`）**

```cpp
virtual bool on_right_click(glm::vec2 pos);          // 右键单击
virtual bool on_double_click(glm::vec2 pos, int button);
virtual bool on_long_press(glm::vec2 pos, int button);
```

默认返回 `false`（不消费），沿用现有冒泡 + 消费机制。

**按钮语义**

- 左键（0）：行为不变 —— 抢焦点、`on_mouse_down/up`、同目标 `on_click`。
- 右键（1）：**不抢焦点、不触发 `on_click`**；释放且 `pressed == target` 时发 `on_right_click`。
  右键抢焦点会让「右键菜单弹出后原控件失焦」等行为难以推理，故明确排除。
- 中键（2）：保持现状（走 `on_click`，由 `button` 参数区分）。

**双击**

- 时间基准：`UISystem` 自己维护 `elapsed_`，在 `tick(dt)` 累加；不使用系统时钟，因此可确定性测试。
- 判定（全部满足）：同一目标、同一 button、距上次 click ≤ `double_click_ms`（默认 400）、
  两次位置距离 ≤ `double_click_slop`（默认 6px）。
- 触发顺序：两次 `on_click` 都照常发，第二次之后再追加一次 `on_double_click`；
  触发后清空 `last_click`，避免三击连发。
- 位置容差用于吸收手抖；不同控件上的两次点击不算双击。

**长按**

- 按下时记录 `press_time / press_pos / press_target / press_button`。
- `tick(dt)` 中检查：仍按住、指针仍在目标 `global_bounds()` 内、`elapsed - press_time ≥ long_press_ms`（默认 600）、
  尚未触发过 → 发一次 `on_long_press`，并标记已触发。
- 指针离开目标范围 → 取消（不再触发）；释放后重置状态。

**配置**

`UISystem::set_double_click_ms(float)`、`set_long_press_ms(float)`、`set_double_click_slop(float)`，
供应用层按平台/设备习惯调整；0 表示禁用对应手势。

### 接入范围（最小）

- `TreeView`：双击行切换展开/折叠（叶子行不响应），与已有的 `Enter`/`Space` 行为一致。
- gallery：`Inputs` 分区新增 `Pointer events` 行 —— 一个目标按钮 + 一个状态标签，
  显示最近一次 click / double / right / long，用于人工验收。
- 其余控件暂不接：右键菜单本体、双击重命名、长按拖拽分别留给后续变更。

### 验证

- 新增 `test_ui_pointer_events`（纯逻辑）：
  - 左键：同目标 down/up → `on_click` 一次且获得焦点。
  - 右键：同目标 down/up → `on_right_click` 一次，**不**触发 `on_click`、**不**改变焦点。
  - 双击：阈值内 → click×2 + double×1；超过 `double_click_ms` → 无 double；位置偏移超过 slop → 无 double。
  - 双击：目标不同 → 无 double。
  - 长按：按下后 `tick` 累计超阈值 → long×1，继续 tick 不重复；释放后不再触发。
  - 长按取消：按下后指针移出目标 bounds，超时也不触发。
  - 消费：子控件消费 `on_double_click` 时父控件不再收到（冒泡验证）。
- 现有 29 个测试全部通过；ASAN 通过。
- gallery 冒烟：`validation=0`。

### 边界（本次不做）

- 不做右键菜单本体（需要 overlay 菜单 + 上下文绑定）。
- 不做三击、多指/触摸、手势识别（缩放/旋转）。
- 不做长按拖拽的视觉反馈（与 `Slider`/`Splitter` 的拖拽反馈分开做）。
- 不改 `on_click` 签名，不引入自定义按钮映射（沿用 GLFW 值）。
- 不改应用层事件桥接（`mouse_button` 已透传）。
