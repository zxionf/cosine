# 20261026-rect-init-fix 任务清单

- [x] T01 `Rect` 显式初始化 `pos` / `size` 为 0
- [x] T02 `Table::measure` 改为 `clamp_size(min_size())`，不读 `global_`
- [x] T03 验证：Containers 截图、ctest 41/41（build + ASAN）、冒烟三组
- [x] T04 决策记录与归档

## 完成定义

- Containers 分区内容完整可见，所有含 Table 的布局不再出现 NaN 尺寸。
- `global_` / `local_` 在首次 arrange 前为 0，不受对象布局变化影响。
- 全量测试与 ASAN 通过，gallery 冒烟 `validation=0`。
