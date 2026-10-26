# 20261026-rect-init-fix 交接

## 结果

- `src/ui/rect.hpp`：`pos` / `size` 显式初始化为 0（`glm::vec2` 默认构造不写零）。
- `src/ui/table.hpp`：`measure` 改为 `clamp_size(min_size())`，不再读 `global_.size`。
- 移除排查用的临时诊断（gallery `COSINE_GALLERY_DUMP` 块、`measure_child` 的 NaN 探针）。

## 验证

- Containers 分区截图：Splitter 双栏、TabView 三页、Table 表头与行均正常。
- `ctest` 41/41（build 与 ASAN）。
- gallery 冒烟：Containers、Layout(classic)、Overlays(line) 三组 `validation=0`。

## 根因笔记

`glm::vec2` 的默认构造**不初始化**（GLM 未定义 `GLM_FORCE_CTOR_INIT`），
而 `Rect` 原先没有默认成员初始化 → `Widget::global_` / `local_` 在首次 arrange 前是垃圾值。

`Table::measure` 读 `global_.size`，首次测量即得到 NaN；`glm::clamp(NaN, …)` 仍是 NaN，
于是内容尺寸 NaN 一路传到 `ScrollView::place_content` → `ScrollPanel` 高度 NaN → 裁剪为空，
只剩面板自身的背景与内阴影。

给 `Widget` 增加字段（absolute / z-index / 百分比 / 宽高比）改变了对象布局，
让 `global_` 落到非零内存，才把这个长期潜伏的缺陷暴露出来。

## 排查方法（可复用）

1. gallery 加一次性 dump（分区 bounds / viewport / content / 每个子项 measure）。
2. `Widget::measure_child` 加 NaN 探针，打印首个返回 NaN 的子项与它的 min/max。
3. 对照 `min/max` 正常但 `measure` 返回 NaN → 说明读的是未初始化的 `global_`/`local_`。

## 后续

- 若再出现「控件在首次 arrange 前被 measure」的场景，优先检查是否读了 `global_`/`local_`。
- M4 按 easing → scene-node → property-system 启动。
