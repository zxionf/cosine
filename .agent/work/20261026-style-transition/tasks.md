# 20261026-style-transition 任务清单

- [x] T01 `UISystem::tick(float dt)` + 无参重载（默认 1/60），`update_tree(dt)` 统一遍历；`Widget::update(float dt)` 非虚入口 + `on_update` 虚钩子
- [x] T02 `Widget::visual_style(role, state)`：`StyleTransition` 状态机（首帧/零时长/中途反向/ease-out quad）
- [x] T03 `Theme::transition_ms`（默认 120）+ `with_transition_ms()`；`lerp_style` 只作用于颜色字段
- [x] T04 第一批控件接入 `visual_style`（Button/Checkbox/Radio/Slider/Progress/TextInput）
- [x] T05 第二批控件接入（Dropdown 框体/Splitter；行级与项级状态不接，见 design 边界）
- [x] T06 gallery：Style 分区 Transition 行（Off/60/120/240）+ `COSINE_GALLERY_TRANSITION` + 主循环传真实 dt
- [x] T07 `test_ui_transition`：零时长、半程插值、到位、中途反向、几何恒定、顶点数不变、确定性、disabled、无参 tick
- [x] T08 全量回归：构建 + ctest 29/29 + ASAN 29/29 + gallery 冒烟 5 组（validation=0）
- [ ] T09 SPEC 折叠与阶段计划/决策记录（缓动曲线配置归大后期）

## 完成定义

- hover / pressed / disabled 切换在 120ms 内平滑过渡，gallery 可人工验收。
- `transition_ms = 0` 时行为与迁移前完全一致（立即到位）。
- 过渡期间顶点/索引数不变，无 validation 错误。
- 全部既有测试通过；新增 `test_ui_transition` 覆盖 6 个场景。
