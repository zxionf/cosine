# M3.5 UI Fixes Handover

状态：用户验证后关闭（2026-10-25）
修复：点击 Dropdown 选项后 hovered_ 悬垂导致段错误。
方案：Widget 生命周期令牌 + UISystem Target{widget, weak_ptr} + tick 前 prune_stale_targets。
验证：19/19 ctest；ASAN 通过；lavapipe 冒烟通过；两项回归测试反向验证有效。
