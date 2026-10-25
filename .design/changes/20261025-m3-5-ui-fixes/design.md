# 20261025-m3-5-ui-fixes

<!-- 为什么改、边界与不属于本次的范围 -->

## 悬垂交互目标

- 症状：`ui_gallery` 中点击 Dropdown 选项后下一次鼠标移动在 `ui_system.cpp` 的 `hovered_->on_mouse_leave()` 段错误。
- 原因：点击选项触发 `Dropdown::close()` → `UIRoot::remove_overlay()` 把浮层移入 `trash_`；下一次 `tick()` 的 `layout()` 销毁它们，但 `UISystem::hovered_` 仍指向已释放的 `DropdownList`。
- 同类路径：`Vbox::clear_children()`、`ScrollView::set_content()`、`RadioGroup::set_options()`、`List::clear_items()` 会**立即**销毁子控件，悬垂窗口更短，仅靠树校验无法覆盖。

## 生命周期令牌

- `Widget` 持有 `std::shared_ptr<const bool> alive_token_`，析构后 `weak_ptr` 过期。
- `UISystem` 的三个交互目标改为 `Target{ Widget*, weak_ptr }`：
  - `valid()` 同时满足指针非空与令牌未过期；
  - 隐式转换到 `Widget*`，`operator->` 与 `explicit operator bool` 让现有事件代码基本无需改写。
- `prune_stale_targets()` 在 `tick()` 开头、`layout()` 之前执行：
  - 令牌过期 → 直接清空（不能再调用虚函数）；
  - 令牌有效但已不在树上（overlay 待销毁）→ 先 `on_mouse_leave()` / `on_focus_lost()` 再清空。

## 边界

- 不改变控件所有权模型（仍由父容器 `unique_ptr` 拥有）。
- 不引入全局控件注册表，避免新增全局可变状态。
- 令牌只用于交互目标校验，不用于控件间通信。
