# 20261026-rect-init-fix

<!-- 为什么改、边界与不属于本次的范围 -->

## Rect 未初始化导致的 NaN 布局

### 现象

`ui_gallery` 的 Containers 分区只显示面板背景与顶部内阴影，内容（Splitter / TabView / Table）
全部不可见；其它分区里含 Table 的地方同样受影响。

### 根因

`Rect` 的 `pos` / `size` 没有默认成员初始化，而 `glm::vec2` 的默认构造**不写零**
（未定义 `GLM_FORCE_CTOR_INIT`）。因此 `Widget::global_` / `local_` 在首次 arrange 之前是未定义值。

`Table::measure` 直接读 `global_.size`：

```cpp
return clamp_size(global_.size.x > 0.0f ? global_.size : local_.size);
```

首次测量时读到未定义值（NaN）→ `glm::clamp(NaN, …)` 仍是 NaN → 表格尺寸 NaN →
父容器 `Vbox` 的内容高 NaN → `ScrollView::place_content` 把内容按 NaN 尺寸 arrange →
`ScrollPanel` 的 bounds 高度 NaN → 视口裁剪为空 → 只剩面板自身的背景与内阴影。

最近的布局改动给 `Widget` 增加了字段（`absolute_` / `z_index_` / `size_percent_` / `aspect_ratio_`），
改变了对象布局，使 `global_` 落在非零内存上，才把这个潜在缺陷暴露出来。

### 修复

1. `Rect` 显式初始化：`glm::vec2 pos{0.0f, 0.0f}; glm::vec2 size{0.0f, 0.0f};`
   —— 根本修复，所有控件的 `global_` / `local_` 在首次 arrange 前为 0。
2. `Table::measure` 改为 `clamp_size(min_size())`，尺寸由构造时设置的 min/max 决定，
   不再依赖 arrange 的执行顺序。

### 验证

- Containers 分区截图：Splitter 双栏、TabView 三页、Table 表头与行均正常显示。
- `ctest --test-dir build`：41/41；`ctest --test-dir build-asan`：41/41。
- gallery 冒烟：Containers、Layout(classic)、Overlays(line) 三组 `validation=0`。

### 边界

- 不改 `glm::vec2` 的全局行为（不定义 `GLM_FORCE_CTOR_INIT`），只在 `Rect` 层显式初始化。
- 不重写 `Dropdown` 内部列表的 `measure`（它返回 `local_.size`，但 arrange 会先 `set_size`，
  当前测试与冒烟均正常）。
