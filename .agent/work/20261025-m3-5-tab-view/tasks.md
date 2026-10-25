# 20261025-m3-5-tab-view 任务清单

## TabView

- [x] T01 `src/ui/tab_view.hpp`：TabBar + 页面容器，`children_[0]` 为标签栏
- [x] T02 切换语义：`apply_selection` 用可见性隔离非当前页
- [x] T03 布局：标签栏顶部、内容区剩余空间、页面默认填满（`set_page_fill`）
- [x] T04 `add_page` / `set_page` / `page` / `set_selected` / `set_on_change`
- [x] T05 新增 `test_ui_tab_view`（可见性、点击/键盘切换、clamp、set_page、回调）
- [x] T06 gallery Containers 分区接入 TabView（三页 + 状态回显）
- [x] T07 全量回归（22/22 ctest、ASAN、lavapipe 冒烟）

## 当前进度

- 完成：22/22 ctest、ASAN、lavapipe 冒烟通过
- 决策 #44：children 暴露标签栏 + 页面、可见性隔离、页面填满内容区
