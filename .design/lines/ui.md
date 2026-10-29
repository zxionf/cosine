# ui 线任务

> 定位：控件、布局、样式、输入路由、`DrawList`、字体与图集。
> 目录：`src/ui/`、`src/graphics/`。编号前缀 `UI`。
> 状态词汇与任务模板见 `.design/阶段计划.md` 的「任务结构」一节。

## 进行中

### UI-6 可拖动面板 / 窗口

- **状态**：`doing`（细节讨论中）
- **背景**：`src/ui/` 只有模态 `Dialog`，没有常驻的浮动面板；编辑器迟早需要可拖可叠的属性窗 / 资源窗
- **目标**：提供 `ui::Window`（标题栏 + 内容区），可拖动、点击置顶、限制在父区域内，样式跟随主题
- **验收**：拖动跟手；点任意位置置顶；拖出父区域时至少保留一段可见；单元测试覆盖拖动/置顶/夹取；冒烟 val=0
- **依赖**：主题新增 `elevated` 令牌（UI-3 的遗留项）
- **步骤**：
  - [x] 定容器层级：挂 `UIRoot`（全窗口区域、不被面板裁剪）+ absolute + z-index；overlay 留给临时浮层
  - [x] `ui::Window`：标题栏拖动、grab 偏移（用 `absolute_pos` 空间，避免父 padding 漂移）、clamp（保留 48px 可见）
  - [x] 置顶策略（按下即置顶）与 z-index 归一化（按 paint 顺序重编 1..N）
  - [x] resize（右下/右/下三个 grip）、折叠（记住展开高度）、两种关闭（隐藏 / 销毁回调）
  - [x] 内部滚动（内容区用 `ScrollPanel`）
  - [x] 测试 `test_ui_window`（6 组：拖动跟手、clamp、置顶、resize、折叠、关闭）
  - [x] `ui_lab` E8 验证：三个窗口叠放，不受面板裁剪
  - [x] `elevated` / `elevated_border` 令牌（深色 `#2A2E33`/`#454B54`，浅色 `#FAFBFC`/`#A8AEB6`）
  - [ ] `ui_gallery` 分区与 `docs/` 更新
  - [x] 双击标题栏最大化/还原（记 `restore_pos_` / `restore_size_`）
  - [x] 8 边 resize（四边 + 四角；顶部抓取带只能 6px，否则吃掉标题栏拖动区）
  - [x] 关闭前确认（`set_on_close` 返回 false 拒绝；Esc 与点叉共用 `request_close`）
  - [x] 焦点环（`has_focus_` → 边框转 `accent` 加粗 2px）
  - [x] 模态（`Scrim` + `UIRoot` 模态栈 + Tab 焦点限制，支持嵌套）

**遗留**（未做，需单独拍板）：

| 项 | 现状 | 前置条件 |
|---|---|---|
| 内容区滚动 | `body_` 是 `Vbox`，超出被 `push_clip` 裁掉；不能直接换回 `ScrollPanel`（它会吞掉按下事件，窗口就不能拖内容区） | 三选一：顶部 24px 专用拖动条 / 仅边缘 8px 可拖 / 内容超高时才启用 |
| 打开/关闭过渡 | 无 | **依赖 BE-2**（`DrawList::push_opacity` 已就位），UI 侧还没接 |
| resize 保留滚动位置 | 无对象可保留 | 依赖「内容区滚动」 |
| 圆角无抗锯齿 | 硬边 | D01（渲染层） |
| 外阴影 | 无 | 渲染层 |
| 边缘吸附 / 多窗口级联 | 无 | 需要窗口管理器概念 |

### UI-7 架构升级（六阶段）

- **状态**：`doing`（阶段 1 进行中）
- **背景**：M6.x 累计暴露四类结构性缺陷——子项所有权无唯一入口、`measure` 契约不明确、文字排版无公共函数、派发链零测试
- **目标**：把「能力靠某个类里再写一遍」改为「能力靠行为基元」
- **验收**：每个阶段独立可交付，进入 ctest 后不得回退
- **已定架构决策**：
  - 子项存储直接放 `Widget`（`children_storage_`），不新增 `ContainerWidget`
  - 层模型用 `enum class Layer { Content, Floating, Modal }`，删掉 `dynamic_cast<Scrim*>`
  - `Theme` 令牌与 `StyleRole` 保留双轨，先补「令牌 → 控件」对照表
- **步骤**：
  - [x] **阶段 1 安全网**：`test_ui_dispatch`（5 用例）+ `test_ui_layout_contract`（6 项），不改任何结构
  - [x] **阶段 2 所有权统一**：`Widget::children_`（protected）+ `adopt()`；11 个容器删自持 `children_`，全项目只剩基类一处
  - [x] **阶段 2b 清理**：所有 `add()`/挂载点改走 `adopt()`，`grep children_.push_back` 只剩基类内部
  - [x] **阶段 2c 微清理**：删 10 处冗余 `children()` 重写，全项目只剩 `section.hpp`（转发 `content_`）
  - [x] **阶段 3 布局契约**：`measure` 契约注释 + `Widget::fixed_size()`；`Table`/`FileBrowser` 标记为 fixed（缓存传播已由阶段 1 验证正常）
  - [x] **阶段 4 层模型**：`Widget::fills_parent()` + `Layer{Content,Floating,Modal}`；`UIRoot::layout_child` 去掉 `dynamic_cast<Scrim*>`
  - [x] **阶段 5 行为基元**：`ScrollState` + `ListView` + `CollapsibleHeader` 均已落地
    - [x] `ScrollState`（`src/ui/scroll_state.hpp`）：clamp / 目标 / 平滑逼近 / range 收敛到一处，6 项单测
    - [x] `Table` 接入 `ScrollState`（13 处引用一次替换，指数衰减/Fast/snap 手感不变）
    - [x] ~~`ScrollView` 接入~~（不迁：二维 `glm::vec2 offset_` + 自带 `scroll_speed`/`line_step`，`ScrollState` 保持一维）
    - [x] ~~`TextInput` 接入~~（不迁：只有一条 `clamp_scroll`、无 `ScrollMode`/平滑，收益为零）
    - [x] 抽 `ListView` 基类（`src/ui/list_view.hpp`：滚动/选中/键盘/可见行，6 项单测）
    - [x] `Table` 接入 `ListView`（继承 + `viewport_height()` 扣表头；`append_to` 不动，行绘制仍自查）
    - [x] 抽 `CollapsibleHeader`（`src/ui/collapsible_header.hpp`），`Section` 已接入
    - [x] ~~`Window` 标题栏复用~~（不迁：标题栏还要拖动/关闭叉/双击最大化，套进去反而更绕）
  - [x] ~~**阶段 6 Widget 瘦身**~~：默认不做，阶段 5 后再评估
- **明确不做**：不重写布局引擎、不引入 ECS、不拆 `Widget`、不动 `menu_bar.hpp`（857 行是菜单语义的合理复杂度）
- **后续优化项**：已独立为 UI-8（constraint 布局）与 UI-9（声明式层）

### UI-8 constraint 布局（Flutter 式）

- **状态**：`todo`（不立即开始，以后做）
- **背景**：现在 `measure(constraints)` 只传「可用宽度」，高度靠子项自报，没有「最小/最大约束 + 子项在约束内收敛」的模型
- **目标**：引入 `BoxConstraints{min_w, max_w, min_h, max_h}`，容器可沿树向下传递
- **触发条件**：出现自适应/流式布局的硬需求（面板任意嵌套、内容驱动宽度）
- **影响面**：`Widget::measure` 契约 + 所有 `measure` 实现
- **验收**：65 项测试全绿 + 一个真实场景（如 ui_lab 自适应列）验证

### UI-9 声明式 / 响应式层

- **状态**：`todo`（不立即开始，以后做）
- **背景**：现在是 retained-mode + 命令回调，控件由命令式代码构造与更新
- **目标**：评估「状态 → UI」的声明式描述（SwiftUI / React 式）是否值得
- **触发条件**：XEL 绑定方向确定后；或界面状态同步代码成为主要 bug 来源
- **风险**：与现有 retained-mode 模型冲突，属于推翻性设计
- **验收**：先出设计文档，不写实现

### UI-10 `draw_row` 接管 + TreeView 迁移

- **状态**：`todo`（不立即开始，以后做）
- **背景**：`ListView::draw_row` 已预留契约，但 `Table::append_to` 仍自己画行；`TreeView` 尚未接入 `ListView`
- **目标**：行绘制搬进 `draw_row`；`TreeView` 继承 `ListView`（缩进/展开自己管）
- **障碍**：`append_to` 的 `snap`（像素对齐）与 `first_visible_row()`（逻辑行号）取整口径不同，合并需统一，且当前无像素级测试
- **前提**：先补「渲染快照对比」手段，否则视觉回归不可见
- **验收**：`Table` 与 `TreeView` 离屏导出对比无差异，65 项测试全绿

### UI-11 主题令牌对照表

- **状态**：`todo`（不立即开始，以后做）
- **背景**：`Theme` 有 30+ 颜色字段，同时存在 `StyleRole` + `ControlState`；控件里两者混用
- **目标**：补一份「令牌 → 控件」对照表，明确每个字段的唯一职责
- **触发条件**：主题需要第三方定制，或出现「改了令牌却不生效」的排查成本
- **验收**：文档层，不动代码

### UI-12 隐式依赖清理

- **状态**：`done`（2026-10-29）
- **背景**：`CollapsibleHeader` 是 `Section` 的普通成员、不在 `children_` 里，`set_context()` 从未被调用，只靠 `Widget::font_atlas()` 的 fallback 兜底；`hit_testable`/`focusable`/`clips_children` 三布尔组合语义全靠约定
- **目标**：消除隐式依赖 + 补行为矩阵文档
- **步骤**：
  - [x] `Section::arrange` 里补 `header_.set_context(context())`
  - [x] `widget.hpp` 注释里补「布尔组合 → 行为」矩阵
- **验收**：`ctest` 全绿；无新增 fallback 依赖

### UI-13 技术债与取舍文档同步

- **状态**：`done`（2026-10-29）
- **背景**：`docs/tech_debt.md` 的 D01–D10 未反映本轮变化（多纹理槽、`centered_text_y`、`draw_row` 空实现等）
- **目标**：同步技术债，并新增「有意为之的取舍」一节，避免下次评估重新纠结
- **步骤**：
  - [x] 更新 D 列表（关闭已解决的、补充新增的）
  - [x] 新增「有意为之」小节（T01–T09）
- **验收**：文档层，不动代码

### UI-14 代码整洁度

- **状态**：`done`（2026-10-29）
- **背景**：`Window` 的 `title_bar_visible_` 有两套分支（`append_to` / `on_mouse_down` / `clamp_position` / `resize_edges` 各一处）；`Table::on_key_down` 与 `ListView` 部分重复
- **目标**：标题栏高度为 0 时自然退化，减少 `if`；`Table::on_key_down` 改成「覆写 + 调基类」
- **验收**：行为不变，`ctest` 全绿

### UI-15 性能观测与候选优化

- **状态**：`todo`
- **背景**：`Table::column_widths` 每帧重算；`Button`/`Table` 单元格每次 `atlas->measure()`；`invalidate_measure()` 一路传播到 root；batch 切分频率未知
- **目标**：先加帧时间 / 测量次数统计，再决定优化哪一项
- **前置**：**先有测量手段**，不做凭感觉的优化
- **验收**：能读出「每帧测量次数」「每帧 batch 数」「帧时间分布」

### UI-3 主体颜色精调

- **状态**：`done`（2026-10-28，见 `.agent/work/20261028-theme-tokens/`）
- **目标**：让主体配色在深色主题下成立：层次可辨、强调色不发白、文字层级清晰；浅色主题同步
- **验收**：导出截图逐区块目视成立；关键配对对比度达标（正文 ≥ 7:1，次级 ≥ 4.5:1，禁用 ≥ 3:1）；`ui_gallery` / `ui_lab` / `cosine` 三个程序冒烟 val=0
- **依赖**：无
- **步骤**：
  - [x] 确定主题数值的颜色空间约定：按 sRGB 书写、存储为线性（`srgb_hex`），验证「写的值 = 显示值」
  - [x] 定一套令牌：shell / surface / control / accent / text×3 / border / splitter
  - [x] 深色主题按新令牌重写（`#1B1D21` / `#232629` / `#41474F` / accent `#3D8EE8`）
  - [x] 浅色主题同步（`#EDEFF2` / `#DEE1E5` / accent `#1F6FD0`）
  - [x] 分割条可见度：新增 `splitter` 令牌，手柄常态不再等同面板底色
  - [x] 时间轴选中行调淡（`accent` 叠加 0.12 → 0.08）
  - [x] 按钮与面板对比：`button_bg` `#3A3F45` → `#41474F`，hover/pressed 同比例上调
  - [x] `accent_text`（`#6FB0F2`）：目录名、选中关键帧、拖动数值等「强调色当文字」的场景改用它
  - [x] 验证：导出截图 + 抽样像素（`#41474F`、`#232629`、`#1B1D21` 均与写入值一致）
  - [x] 更新 `.design/SPEC.md` 的主题节与 `docs/`（新增「颜色空间与令牌」节）

## 待开始

### UI-5 控件转正（由 `ui_lab` 结论驱动）

- **状态**：`blocked`（等 `EX-2` 的实验结论）
- **目标**：把 `ui_lab` 中被验证的组合交互提升为正式控件
- **验收**：新控件必须有单元测试 + `ui_gallery` 分区；`ctest` 全绿、gallery 冒烟 exit=0 / validation=0
- **依赖**：`EX-2`
- **步骤**：
  - [ ] 从 `examples/ui_lab/README.md` 选择已得出结论的实验
  - [ ] 设计正式 API（复用 lab 的交互结论，不照抄代码）
  - [ ] 实现到 `src/ui/`，补单元测试
  - [ ] 在 `ui_gallery` 加分区 + 更新 `test_ui_gallery`
  - [ ] 更新 `.design/SPEC.md` 与 `docs/`

### UI-4 TreeView 可见行缓存（M4.6 T13）

- **状态**：`paused`
- **目标**：可见行不再每次绘制重建（当前含 path 拷贝），节点上千后仍流畅
- **验收**：1000 节点展开时滚动无卡顿；行为与现状一致
- **依赖**：无
- **重启条件**：场景节点规模接近上千
- **步骤**：
  - [ ] 维护扁平可见数组 + 失效版本号
  - [ ] 折叠父级保留子树展开状态的语义保持不变
  - [ ] 补滚动/展开回归测试

## 已完成

| 编号 | 内容 | 完成 | 记录 |
|---|---|---|---|
| UI-0 | M2 GUI 框架 MVP（事件、布局、文本与字体、draw list、裁剪） | 2026-10-24 | `docs/plan_archive.md` → M2 |
| UI-1 | M3.5 组件扩充 + 样式系统 + M3.6 滚动三模式 | 2026-10-26 | `docs/plan_archive.md` → M3.5 / M3.6 |
| UI-2 | M4.6 T1–T4、T8（图集空间、脏矩形、测量缓存、行高、DPI） | 2026-10-27 | `docs/plan_archive.md` → M4.6 |
