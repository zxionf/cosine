# 20261025-m3-5-ui-fixes 任务清单

## 悬垂交互目标修复

- [x] F01 `Widget` 增加 `alive_token()` 生命周期令牌
- [x] F02 `UISystem` 交互目标改为 `Target{ widget, weak_ptr }`，提供 valid/operator->/bool
- [x] F03 `prune_stale_targets()` 区分“已销毁”与“已移出树”两类失效
- [x] F04 回归测试：事件系统点击 Dropdown 选项后不悬垂
- [x] F05 回归测试：`clear_children()` 立即销毁后 hover 目标被清理
- [x] F06 全量回归（ctest / ASAN / lavapipe 冒烟）

## 当前进度

- 根因：`Dropdown` 关闭后 overlay 在 `layout()` 被销毁，`hovered_` 悬垂
- 修复：生命周期令牌 + tick 前 prune；19/19 ctest、ASAN、lavapipe 冒烟通过
