# 已归档的阶段计划（M1 – M6.4b）

> 本文件保存 `.design/阶段计划.md` 中**已完成阶段**的计划原文，供追溯「当时怎么定的、验收什么」。
> 当前路线与未完成计划只看 `.design/阶段计划.md`；已完成基线的结论见 `docs/history.md`；
> 当前有效设计事实见 `.design/SPEC.md`。
>
> 归档规则：某阶段的条目**全部完成**（或只剩人工走查）后，原文整体搬到这里，
> 原处只留一行摘要与指针。原文保持原样、不随代码演进改写；若与现状不符，
> 以 `docs/SPEC.md` 与 `docs/` 下的专题文档为准。

## 索引

| 阶段 | 完成时间 | 一句话摘要 |
|---|---|---|
| M1 基座稳定 | 2026-10-24 | 修完 P0/P1，构建 / 资源 / 事件 / 布局可移植 |
| M2 GUI 框架 MVP | 2026-10-24 | 裁剪、draw list、文本与字体、`ui_demo` 收口 |
| M3 MVP 收口与工程化 | 2026-10-25 | `UIContext`、CI、ASAN、指标、文档、打包 |
| M3.5 UI 打磨与组件扩充 | 2026-10-26 | 组件补齐、样式系统、交互打磨、`ui_gallery` |
| M3.6 滚动体验 | 2026-10-26 | Smooth / Fast / Line 三种滚动模式 |
| M4 2D 动画内核 | 2026-10-26 | 场景图、属性、时间轴、缓动、播放器、事件 |
| M4.5 动画能力全面展示 | 2026-10-26 | `ui_anim_showcase` 三展区动画展厅 |
| M4.6 已完成条目（T1–T4、T8） | 2026-10-27 | 图集空间、脏矩形上传、测量缓存、行高语义、DPI |
| M6.0 离屏渲染与图像导出 | 2026-10-27 | 离屏渲染 + PNG 导出 |
| M6.1 项目序列化与撤销 | 2026-10-27 | 二进制项目文件 + 撤销栈 |
| M6.2 属性编辑控件与 2D 旋转 | 2026-10-27 | `DragFloat` / `ColorInput` / 2D 旋转 / 属性面板 |
| M6.3 时间轴编辑视图 | 2026-10-27 | 标尺、轨道、关键帧、吸附、框选、Player 联动 |
| M6.4a 编辑器骨架 | 2026-10-27 | 编辑器外壳、三栏布局、classic 默认外观 |
| M6.4b 编辑器联动 | 2026-10-28 | 选择联动、编辑入栈、可拖分区、预览标尺、多选 |

> 各节标题与上表名称一一对应，按名称检索即可。未完成部分（M4.6 剩余 T5–T13、
> M6.5 剩余、M6.6、M6.7、M5、M7、M8）留在 `.design/阶段计划.md`，不在本文件。

---

## M1 基座稳定（已完成，2026-10-24）

**目标**：把“能跑”变成“能持续开发”。先修正确性与工程化，不碰动画功能。

> 完成状态与任务清单见 `.agent/work/20261024-m1-stability/tasks.md`。
> 文本测量、HiDPI、ScrollView 渲染裁剪栈延后到 M2。

---

## M2 GUI 框架 MVP（已完成）

**目标**：做出一个真正可交互、可扩展的 2D GUI 框架，不依赖动画能力。

> 实际状态见 `.agent/work/20261024-m2-gui-mvp/tasks.md`。
> M2-A/B/C/D 已完成；剩余 M2-E 渲染裁剪、M2-F demo/应用层收口。
> 评估与债务归档见 `docs/project_assessment.md`。

### M2-A/B/C 已完成（摘要）

- 事件：命中路径、冒泡/消费、鼠标捕获、click、焦点、Tab、key/char、HiDPI 光标换算、on_hover/debug。
- 布局：Vbox/Hbox/Spacer、padding/margin、flex/grow、min/max、主轴分布、measure 缓存、ScrollView 双轴。
- 控件：Theme、FontAtlas、Label、Panel、Button、Checkbox、Slider、TextInput、List、Splitter、Tooltip。

### M2-04 文本与字体（已完成）

**已定方案**

- 风格：统一 TTF（A2）；`default8.png` 仅作 TTF 缺失 fallback。
- 来源：系统字体优先，`COSINE_CJK_FONT` 覆盖；打包阶段可选内置 OFL 字体。
- 覆盖：ASCII + GB2312 一级汉字（3,755）+ 常用标点（C2）。
- 图集：固定 2048×2048 RGBA8，16px 基准；其他字号先按比例缩放。
- 生成：运行时 FreeType 生成并上传；D3 按需追加字形并更新纹理已落地（当前整图重传，dirty-rect 后续优化）。
- 度量：ascent/descent/line_gap + bearing/advance，按基线排版。
- 采样：TTF 图集用 linear；像素 fallback 用 nearest。
- 后续：完整 CJK/动态按需图集、SDF/MSDF、IME preedit、HarfBuzz。

**任务**

- [x] FontAtlas 改为 codepoint 维度，增加 UTF-8 解码。
- [x] TextInput 按 codepoint 维护光标/选择，修复水平滚动后的点击索引。
- [x] `measure` / `append` 支持自动换行、最大宽度、行高、省略号。
- [x] 文本绘制与 TextInput 裁剪接入 TextClip（完整 scissor 栈在 M2-E）。
- [x] 基础 FontAtlas / TextInput / 换行测试。
- [x] FreeType 链接、字体发现（Linux/Windows/env）与 `FontAtlasBuilder`。
- [x] 生成 ASCII + GB2312 一级 + 标点的 2048×2048 图集，Renderer 上传。
- [x] 字体度量（ascent/descent/line_gap/bearing/advance）与基线排版。
- [x] TTF 缺失时回退 `default8.png`。
- [x] 图集格式、字体来源、许可证、字号策略文档化（docs/font_atlas.md）。
- [x] D3 增量字形更新（运行时按需追加并更新纹理）。
- [x] CJK 覆盖测试、动态字形测试、无字体回退测试、生成耗时记录。
- [x] SDF/MSDF 与多字号评估：暂不引入，见 docs/font_atlas.md。

### M2-05 渲染与裁剪（完成）

- [x] 引入统一 `DrawList` / `DrawCmd`（vertex/index offset、count、scissor、texture）。
- [x] 裁剪栈：容器 push/pop scissor，渲染层合并裁剪区域。
- [x] ScrollView、TextInput、Panel、Label、Splitter 使用自动裁剪。
- [x] 按 texture/scissor 切批，减少状态切换。
- [x] 图层 / z-order：UIRoot overlay 层，弹窗、Tooltip 先命中、后绘制。
- [x] 保留 `WHITE` 路径，为离屏 render target 预留抽象（`RenderTarget`）。
- [x] 增加裁剪/draw list 测试与 validation 冒烟。

### M2-06 应用层与 demo 收口（完成）

- [x] CMake 共享源去重（`cosine_text` / `cosine_ui`），CI 默认 `-j2`，测试关闭 module scan。
- [x] 删除 `ex4` 手动 `scroll->on_scroll/measure/arrange`，统一走 UISystem。
- [x] 把 Renderer 的 `draw_frame`、F11 截图、空 `process_mouse_move` 移出应用层。
- [x] 形成 `examples/ui_demo`，作为 M2 验收对象。
- [x] 归档/删除 `ex2`、`ex4` 与未使用资源/代码。
- [x] 清理后的 demo 连续运行 60 秒，validation 无错误。

### M2 验收标准

- GUI demo 包含面板、按钮、复选框、滑块、文本输入、滚动列表，所有交互可用。
- 文本支持 ASCII + 常用 CJK，换行/省略号/裁剪正常。
- ScrollView、TextInput、Panel 内容不会画出 viewport。
- Tab 可遍历焦点，按钮可回车/空格触发。
- HiDPI 在 1x / 1.5x / 2x 下表现一致。
- 连续交互 60 秒，Vulkan validation 无错误。
- 布局/事件/文本/裁剪/draw list 测试全部通过。

---

## M3 MVP 收口与工程化（已完成；Windows 实测待补）

- [x] 引入 `UIContext`（FontAtlas/Theme/资源）替代全局 active font/theme，改善多窗口与测试。
- [x] 公共头文件整理：`ui/`、`renderer/`、`app/` 的 include 边界；UI 不再直接依赖 GLFW 键码。
- [x] 调试面板：FPS、frame time、draw call、vertices/indices、hover/focus 节点。
- [x] 错误处理：资源缺失、shader 编译失败、Vulkan 初始化失败给出明确信息。
- [x] CI 矩阵：Linux + Windows；Linux 跑 ctest、lavapipe 冒烟与 ASAN。
- [ ] Windows 构建实测并修复平台差异（工作流与打包脚本已就绪，待首次远端/真实环境运行）。
- [x] 打包：Linux 可执行目录 + assets；Windows zip。
- [x] README 更新（按 CONTRACT 由用户执行，AI 提供草稿：`docs/readme_draft.md`）。
- [x] 清理未使用资源和代码（已随 M2-F 完成）。
- [x] 版本号（0.3.0，CMake 生成 `version.hpp`）与发布说明模板（`docs/release_notes_template.md`）。

**验收**：从干净 clone 到运行 GUI demo 的步骤可在 Linux / Windows 复现；CI、ASAN 与 lavapipe 通过；MVP 可交付。

> 归档注记：Windows 实测一项转入技术债 R09 跟踪，其余条目已完成。

---

## M3.5 UI 打磨与组件扩充（已完成，2026-10-26）

**目标**：把“能用”的控件集变成“顺手”的 UI 框架。M4 动画内核会依赖这些控件做调试面板，
后续 M6 时间轴编辑器也直接复用，所以先补齐组件、样式与交互细节。

**边界**：只做 UI 层；不引入第三方 UI 库；不实现窗口装饰/系统对话框；动画属性系统仍属 M4。

### 组件扩充

- [x] `TabBar`：标签栏，点击或 Left/Right/Home/End 切换。
- [x] `TabView`：标签栏 + 页面容器，切换页面用可见性隔离，页面默认填满内容区。
- [x] `Dropdown`：下拉选择，overlay 遮罩 + 列表，支持方向键/Home/End/Enter/Esc、点击遮罩或再次点击关闭。
- [x] `Radio` / `RadioGroup`：单选组，菱形标记，点击或空格选中。
- [x] `Progress`：确定进度条与不确定态（条带循环滑动，1.2s 周期，由 `on_update(dt)` 推进）。
- [x] `Image`：占位实现（`source` / `texture_id` / `fit` / `tint` / 占位文案），绘制为底色 + 边框 + 对角交叉 + 文件名；真实纹理采样留到 M6。
- [x] `Menu` / `MenuBar`：标题栏 + overlay 菜单，快捷键提示、禁用项、分隔线、键盘导航、多层子菜单（Right/Left 进出、Esc 全关）。
- [x] `Dialog` / `Modal`：卡片（标题/正文/按钮）+ 全屏遮罩，Esc、遮罩点击关闭，可禁用。
- [x] `Table`：固定表头、弹性列、选中/hover/斑马纹、滚轮滚动与自绘滚动条。
- [x] `TreeView`：层级缩进、展开/折叠、键盘导航。
- [x] `Tooltip` 增强：延迟显示、跟随光标、边界翻转。

### 样式系统

- [x] Theme 扩展：圆角（含 `radius_pill` 胶囊档位）、内阴影、强调色、密度（compact/normal/spacious）、字号档位。
- [x] 亮色主题与主题切换（运行时可切换：`UIRoot::set_theme`、`UISystem::set_theme`、gallery 按钮与 `COSINE_GALLERY_THEME`）。
- [x] 控件状态样式统一：`ControlState` + `StyleRole` → `BoxStyle` 的唯一派生入口；焦点环不进填充状态（颜色插值见「UI 动画」）。
- [x] 焦点环与键盘可见性：`FocusStyle{Outline,Border}` × `FocusPolicy{Keyboard,Always,Never}` 自由组合；classic 保留直角 + 贴边焦点框。
- [x] 全部控件迁移：第一批 Button/Checkbox/Radio/Slider/Progress/TextInput；第二批 Panel/MenuBar/Dropdown/TabBar/Table/TreeView/Tooltip/Dialog/滚动条/Splitter。

### 交互打磨

- [x] 键盘导航：`Widget::set_tab_order`（>0 显式顺序 / 0 树序 / <0 跳过 Tab）；RadioGroup roving tabindex（组占一个停靠点，Up/Down/Home/End 切换）；TabBar 与 Dropdown 的方向键已于组件扩充阶段具备。
- [x] 双击、长按、右键菜单预留事件：`on_double_click` / `on_long_press` / `on_right_click`，阈值可配置；右键不抢焦点、不触发 `on_click`。
- [x] 拖拽：Slider 手柄拖动时显示数值气泡与强调色环；Splitter 拖动手柄高亮，ratio 由子项 `min_size` 约束上下限。
- [x] 文本输入：多行（Enter 换行、Up/Down 跨行、Home/End 行内）、跨行选区；Ctrl+Z/Y 与多行水平滚动条待补。
- [x] 滚动条：ScrollView 可见滚动条、滑块拖拽、轨道翻页、按需预留空间。

### 文本显示（M3.5 追加）

- [x] `graphics::TextDebugOptions` + `append_text_debug`：字形盒、行框、基线、advance 刻度，与绘制同源。
- [x] `Label` 排版选项：`TextAlign`、像素对齐（`set_pixel_snap`）、行距（`set_line_spacing`）、调试层开关。
- [x] `ui/typography.hpp`：`TextRole` / `text_style` / `Heading` / `Paragraph`。
- [x] `FontAtlas` 行距语义：`measure` / `measure_wrapped` / `append_text*` 支持 `line_spacing`，测量与绘制一致。
- [x] `ui_demo` 改为文字实验台：角色、对齐、换行、行距、CJK 样例，F1–F5 切换调试层。
- [x] 换行宽度取「可用宽度 ∩ 控件 max_size」，修复测量与 arrange 宽度不一致导致的溢出。
- [x] 按字号栅格化：字形 key = (字号, codepoint)，`FontFace::add_glyph` 按目标像素尺寸重建，图集不足时回退基准字号。
- [x] 裁剪矩形向外取整（floor 左上 / ceil 右下），避免亚像素降部被截断一行。
- [x] 行内文字竖直居中统一用 `FontAtlas::line_height(size)`（菜单项、单行 TextInput），不再用字号近似。

### 布局补充

- [x] `Grid`：列宽支持等分与「固定 + 弹性权重」混合（`set_columns` / `set_column_widths`，正=固定、非正=弹性权重），行高按内容自动取最大，`set_cell_align` 控制格内对齐（默认横向填满）。
- [x] `Wrap` / Flow 布局：子项按可用宽自动折行；`row_align`（Start/Center/End/SpaceBetween）+ `cross_align` 行内垂直对齐；单个超宽子项独占一行。
- [x] 百分比尺寸与 aspect ratio：`Widget::set_size_percent` / `set_aspect_ratio`，容器统一经 `Widget::measure_child` 解析；无界约束下百分比不生效，百分比优先于 stretch。
- [x] 绝对定位与层叠顺序（z-index）显式接口：`Widget::set_absolute`（相对父内容区、不参与流式尺寸）+ `set_z_index`（绘制升序/命中降序，`paint_order` 共用）。

### UI 动画（轻量）

- [x] 悬停/按下过渡（颜色插值）：`Widget::visual_style` + `StyleTransition`，`transition_ms` 四档（Off/60/120/240）；只插值颜色、几何瞬时切换。
- [ ] 面板/弹层出现消失过渡（需 alpha 合成与可见性生命周期改造）—— 明确推迟到 UI 打磨收尾之后再做，不阻塞 M4。
- [x] 与 M4 时间轴解耦：UI 内部 `update(dt)` 推进，不依赖动画内核。
- [ ] 缓动曲线可配置（linear / ease-in/out / cubic-bezier），大后期优化，不阻塞 M4。

### Demo 与验收

- [x] 新增 `examples/ui_gallery`：分区展示全部控件、焦点遍历、overlay、主题切换。
- [x] `test_ui_gallery`：纯逻辑验证分区切换、控件状态、tooltip 显示、滚动内容。
- [ ] 每个组件补充对应单元测试。

**验收**：`ui_gallery` 可在 Linux 稳定运行，全部控件可见可交互；主题切换即时生效；
Tab 可遍历所有可聚焦控件；validation 无错误。

> 归档注记：三条未勾选项（弹层过渡、缓动可配置、逐组件单测）未随 M3.5 完成，
> 已并入技术债与后续阶段，不再阻塞路线。

---

## M3.6 滚动体验（2026-10-26，已完成）

- [x] 平滑滚动：`ScrollMode{ Smooth, Line }`；滚轮只改目标偏移，实际偏移按时间常数指数逼近
  （帧率无关、0.25px 吸附、不越界）。
- [x] 离散按行滚动：`ScrollMode::Line` 立即到位；`ScrollView` 用像素步长，
  `TreeView` / `Table` 用行步长（`row_height × 1.5`）。
- [x] 统一接入 `ScrollView` / `TreeView` / `Table`；平滑期间只平移内容、不触发整树重排。
- [x] gallery `COSINE_GALLERY_SCROLL_MODE=line|smooth`。

> 归档注记：后续在 M6.4b 期间补入 `ScrollMode::Fast`（更小时间常数 + idle 后减半）。

---

## M4 2D 动画内核（已完成，2026-10-26）

- [x] 场景图 / 节点模型：`scene::Scene` / `Node`（Group、Shape、Text、Image、Camera），稳定 `NodeId`。
- [x] 属性系统：`anim::PropertyValue`（variant）+ 内置 `prop::*` 常量 + `PropertyRegistry`，`Scene::get/set_property`。
- [x] 时间轴：`anim::Timeline`（duration、fps、play/pause/seek、循环、时间缩放）。
- [x] 轨道与关键帧：`Track` / `Keyframe`，采样时按区间终点缓动插值。
- [x] 缓动：`anim::Easing`（Linear / EaseIn / EaseOut / EaseInOut / 自定义 cubic-bezier）。
- [x] 播放器：`anim::Player`（tick/seek/apply），确定性 seek，渲染与时间解耦。
- [x] 事件/回调：`Player::add_event`，单点触发、循环回绕重新生效，`seek` 不触发。
- [x] 序列化前置：`NodeId` 与 `PropertyId` 稳定（自增不复用 / 编译期常量）。

**验收**：`ui_anim_demo` 由 Scene + Timeline + Player 驱动，可暂停/重置，逐帧确定性；
周期 3s 循环展示（等价于 10s 连续播放）。渲染入口 `ui::SceneView`。

---

## M4.5 动画能力全面展示（已完成，2026-10-26）

**目标**：用一个完整的「动画展厅」example 全面展示 M4 的能力，作为 M5 之前的验收与对外演示。

**交付**：`examples/ui_anim_showcase`

- 场景：多层级 Group + Shape/Text，覆盖变换累积；
- 属性：position / scale / color / text / visible 的关键帧；
- 缓动：Linear / EaseIn / EaseOut / EaseInOut / 自定义 cubic-bezier；
- 时间轴：播放 / 暂停 / 循环 / 速度 / 前后跳转；
- 事件：时间点触发底部事件标签。

**验收**：展厅可交互播放与跳转；三个展区分别对应缓动、层级、属性动画。

---

## M4.6 已完成条目（T1–T4、T8）

以下为 M3.5 遗留的优化项，已经完成的五条归档于此；**未完成的 T5–T7、T9–T13 仍留在
`.design/阶段计划.md`** 的 M4.6 段落里。

- [x] T1 图集空间治理：`add_glyph` 区分「空间不足」与「缺字」，空间不足不再写入 `missing_`（可重试）；新增 `used_ratio()` 监控水位。字形淘汰/重建与多纹理槽留待后续。
- [x] T2 纹理上传优化：`FontAtlas` 维护脏矩形，渲染层按矩形做紧凑 staging + 区域 `vkCmdCopyBufferToImage`；单字形上传量从 16MB 降到几十 KB。
- [x] T3 字形缓存命中与测量性能：字形命中本就是 O(1)；为 `Label` 加测量缓存（key 含 text/size/wrap 宽度/行距/行数/图集），左对齐绘制跳过测量。
- [x] T4 基线/行高语义收紧：新增 `FontAtlas::normalize_metrics`，`line_height = max(height, ascent + |descent|, 1.0)`；`begin_generated` 与 `FontFace::metrics` 统一走它。
- [x] T8 字号与 DPI：引入 `ui_scale`（默认 1.0，`COSINE_UI_SCALE=auto|数值`）；root / 输入 / 投影 / scissor 统一按逻辑像素，字形按 `size × ratio` 栅格化（上限 256）。

---

## M6.0 离屏渲染与图像导出（已完成，2026-10-27）

- [x] 离屏 `RenderTarget`：创建 offscreen image/view，复用同一 `begin_rendering` / `end_rendering`。
- [x] 读回：`vkCmdCopyImageToBuffer`，`bufferRowLength = 0` 紧凑布局，读回后 BGRA → RGBA。
- [x] PNG 写出：`stb_image_write`（`STB_IMAGE_WRITE_IMPLEMENTATION` 在 `renderer.cpp`）。
- [x] 验收：`ui_anim_showcase` 的整棵 UI 树渲染到 1200×800 PNG，画面与窗口一致、validation=0。

---

## M6.1 项目序列化与撤销（已完成，2026-10-27）

格式见 `docs/project_format.md`（头部 16 字节、主/次版本、小端、节点表 + 时间轴元数据 + 轨道表 + `END!` 标记）。

- [x] 数据模型：`Project{ Scene, Timeline }`，只用稳定 id（`NodeId` / `PropertyId`）。
- [x] 二进制读写：按 `docs/project_format.md` 实现；未知版本、截断、层级成环都给出明确错误。
- [x] round-trip 测试：写入→读回逐字段相等（含嵌套 Group、多轨道、阶跃值、自定义贝塞尔）。
- [x] 撤销栈：`Command` 接口（`apply` / `revert`）+ `CommandStack`，含增删节点、改属性、增删关键帧五个命令。
- [x] 验收：`test_anim_serialize`（8 组）+ `test_anim_command`（8 组），ctest 53/53。

---

## M6.2 属性编辑控件与 2D 旋转（已完成，2026-10-27）

- [x] 数值输入：`DragFloat`（拖动 + 键盘 + clamp/step/精度/后缀/禁用）。
- [x] 颜色输入：`ColorInput`（预设色板 + hex 只读 + 键盘循环），回调不回写 alpha。
- [x] `SceneView` 支持 2D `rotation`：绕节点中心、父旋转累积；椭圆与文字暂不旋转。
- [x] 编辑器属性面板接通（选中联动 + `FocusPolicy::Always`）。
- [x] 验收：gallery 第 9 分区 `Properties`；ctest 55/55。
- [ ] `SceneView` rotation 的单元测试（当前靠编辑器截图人工验收）。

> 归档注记：末条未完成，随 M6.4b 一起仍靠人工验收，已作为测试缺口记录。

---

## M6.3 时间轴编辑视图（已完成，2026-10-27）

- [x] 时间标尺 + 轨道行 + 关键帧菱形，时间↔像素映射（`set_pixels_per_second`）。
- [x] 点击 seek + 拖拽关键帧（`Track::set_key_time` 保序重插）+ 吸附（整秒 / 其它关键帧，6px）。
- [x] 右键删除关键帧（空轨道一并移除）、双击循环切换插值。
- [x] 框选多选与批量移动（按 delta 方向排序防碰撞）。
- [x] 与 `Player` 联动：播放驱动预览与游标，点标尺 seek 同步 Player。
- [x] 验收：拖关键帧→存盘→读回一致（`test_edit_then_round_trip`）。

---

## M6.4a 编辑器骨架（已完成，2026-10-27）

- [x] 产品位置：`src/cosine/`（`cosine_editor` 静态库 + `cosine` 可执行）；`COSINE_BUILD_EDITOR` 可关。
- [x] 菜单（文件/编辑/视图/播放）+ 工具栏（Play / Reset / Add Shape / Style）。
- [x] 布局：左场景树（绑 `Project` 层级）、中预览、右属性占位、下时间轴占位、底部状态栏；窗口自适应。
- [x] 默认 classic 外观（直角、无过渡）；圆角现代样式作为实验项（View 菜单 / Style 按钮）。
- [x] 验收：`COSINE_EXIT_AFTER=3 ./build/src/cosine/cosine` exit=0 / validation=0；ctest 53/53。

---

## M6.4b 编辑器联动（2026-10-28）

- [x] `Splitter` 可拖分区（上下 / 左右 / 左右三份嵌套），并支持固定一侧像素宽度，
      三栏互不影响（树 280 / 属性 380 / 预览弹性）。
- [x] 选择联动：场景树选中 → 属性面板 → 时间轴轨道行高亮。
- [x] 菜单接 `CommandStack`：属性拖动可 Undo/Redo（`SetNodeFieldCommand`）；Playback 菜单已接 Player。
- [x] 时间轴编辑入命令栈：拖动（`MoveKeyCommand`）、删除（`RemoveKeyCommand`）、切插值（`SetKeyEasingCommand`）。
- [x] 颜色选择与键盘微调入栈（`ColorInput` / `DragFloat` 的 commit 回调 + `SetNodeColorCommand`）。
- [x] 预览时间标尺与逐帧（`PreviewRuler` + `PreviewPane`；`◀/▶` 按 1/30s 步进并自动暂停）。
- [x] 多选：Shift/Ctrl 点击切换、Shift/Ctrl 框选追加、多选拖动整组一条撤销（`CompoundCommand`）。
- [ ] 验收：新建→加节点→调属性→拖关键帧→播放，全流程无卡点（脚本化验证已过，待人工走查）。

> 归档注记：实现条目全部完成，仅剩一次人工全流程走查。
