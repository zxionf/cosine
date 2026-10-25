# 20261025-m3-5-tab-view

<!-- 为什么改、边界与不属于本次的范围 -->

## TabView

- 新控件 `TabView`：上方 `TabBar` + 下方内容区，只显示当前选中页面。
- 页面以 `children_` 暴露（`children_[0]` 为标签栏，其余为页面），
  这样命中测试、Tab 焦点遍历、布局脏标记都能复用现有机制。
- API：
  - `add_page(label, widget)` → 返回页面下标（0 起）
  - `set_page(index, widget)`、`page(index)`、`page_count()`
  - `set_selected(index)` / `selected()`（直接切换并触发 `on_change`）
  - `set_on_change(std::function<void(int)>)`
  - `tab_bar()` 访问底层标签栏
  - `set_page_fill(bool)`：页面是否填满内容区（默认 true）
- 切换语义：`TabBar::set_selected` → `TabView::apply_selection` →
  非当前页 `set_visible(false)`，因此不可见页面不参与命中与焦点遍历。
- 布局：标签栏按自身测量高度占顶部，剩余空间减去 padding 后给页面；
  所有页面按同一内容矩形 arrange（切换无额外布局抖动）。
- 绘制：自绘底部/右侧边框，内容区裁剪到自身范围。

## 边界

- 不做标签拖拽重排、关闭按钮、可滚动标签栏、懒加载页面。
- 不与 M4 动画/时间轴耦合。
