# 20261026-style-system 任务清单

## 第一批（本次交付：基础设施 + 高频控件）

### 基础设施

- [x] T01 `ui/style.hpp`：`StyleRole` / `ControlState` / `BoxStyle` / `control_state()`；`Theme::style()` 实现，缺省回落规则
- [x] T02 `graphics/box.hpp`：`append_rounded_rect`（九宫格不重叠 + 角自适应分段 + 小半径退化）
- [x] T03 `graphics/box.hpp`：`append_rounded_ring`（边框/焦点环共用）、`append_inset_shadow`
- [x] T04 `Theme` 扩展：`accent`、圆角档位、`heading_text_size`、`Density` + `with_density()`、`inset_shadow`、`text_input_border_hover`、`track_hover`；dark/light 两套取值补齐
- [x] T05 `UIContext::input_mode` + `UISystem` 维护 Mouse/Keyboard；`Widget::focused_`/`set_focused()`/`focus_visible()`/`append_focus_ring()`，控件删自有 `focused_`
- [x] T05b `Widget::set_corner_radius()`：单控件圆角覆盖（gallery 展示与局部特殊形状）
- [x] T05c 几何风格选项：`FocusStyle{Outline,Border}` + `Theme::with_rounding(bool)`；`Widget::append_focus_ring` 按风格分派（圆角环 / 贴边四框）
- [x] T05d 焦点时机选项：`FocusPolicy{Keyboard,Always,Never}` + `Theme::with_focus_policy()`；与几何正交，`focus_visible()` 只看 policy

### 第一批控件迁移

- [x] T06 Button、Checkbox、Radio、Slider、Progress、TextInput 改用 `style()` + 圆角 + 统一焦点环
- [x] T06b 上述控件删除自有 `hovered_`/`focused_`，改读基类 `is_hovered()`/`is_focused()`；`Button::set_pressed()` 供伪状态展示
- [x] T06c 经典样式下行为等价：圆角归零后绘制回到 4 顶点矩形，焦点框回到贴边四框
- [x] T06d 预设组合：经典 = `with_rounding(false)+Always`，现代 = `with_rounding(true)+Keyboard`；其余组合由两维自由拼
- [x] T06e 圆形控件跟随开关：新增 `Theme::radius_pill`（大值 + clamp），Slider 轨道/手柄、Progress、Radio 用它；classic 下回到直角/方形
- [x] T06f Checkbox 像素对齐：外框/边框/勾选块 round 到整像素，inset 取整后四边留白相等

## 第二批（列表与浮层，已完成）

- [x] T07 Panel、MenuBar、Dropdown、TabBar、Table、TreeView、Tooltip、Dialog、ScrollView 滚动条、Splitter
- [x] T07b 第二批控件启用 `Row` / `MenuItem` / `Tooltip` 角色与 `row_height` 密度缩放，并接上 `append_inset_shadow`

## 验证与展示

- [x] T08 `test_ui_style`：样式解析、状态优先级、密度缩放（含可逆与幂等）、圆角/环带几何（顶点数、索引合法、全在矩形内、半径 clamp）、焦点 2×3 组合矩阵、经典样式几何（贴边四框 16 顶点）、圆形控件退化（slider 12 / progress 24 / radio 26 顶点）、Checkbox 像素对齐（整数顶点 + 同心 + 四边留白相等）
- [x] T09 现有测试回归：28/28 通过（`test_ui_theme`、`test_ui_widgets`、`test_ui_controls`、`test_ui_gallery` 等）
- [x] T10 gallery `Style` 分区：五态对照、圆角覆盖 0/4/10/18、密度三档、强调色样例、Rounded/Classic 与 Keyboard/Always/Never 两行独立选项；导航栏 `Style`/`Focus` 两个切换按钮与 `COSINE_GALLERY_CLASSIC`/`COSINE_GALLERY_FOCUS` 环境变量
- [x] T11 全量构建 + ctest 28/28 + ASAN 28/28 + gallery 冒烟（默认/经典、深色/浅色组合均无 validation 错误）
- [ ] T12 SPEC 折叠（`sele close`）与 `阶段计划.md` 样式系统条目勾选

## 完成定义（第一批）

- 第一批 6 个控件的背景/边框/焦点环统一走 `BoxStyle` 与绘制辅助，不再各自拼 `Rectangle` 与颜色 if 链。
- 圆角在 Button/Checkbox/Radio/Slider/Progress/TextInput 上可见；半透明圆角无接缝变亮。
- 键盘 Tab 聚焦显示焦点环，鼠标点击后隐藏。
- 密度 compact/normal/spacious 切换后度量与字号同步变化，gallery 可人工验收。
- 经典样式（`Style: Classic` 或 `COSINE_GALLERY_CLASSIC=1`）与样式系统之前的观感一致：直角、贴边焦点框，且鼠标点击聚焦也会出现边框。
- 几何（Rounded/Classic）与焦点时机（Keyboard/Always/Never）可自由组合，6 种组合均能正确绘制或隐藏焦点。
- 经典样式下不再有任何圆形元素：slider/滚动条式控件、progress、radio 全部为直角或方形。
- 构建零新增警告，ctest 与 ASAN 全绿。
