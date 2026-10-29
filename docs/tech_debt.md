# 技术债

> 只列「已经知道欠着、暂时不打算还」的东西。删行即表示已还清，并应在
> `.design/DECISIONS.log` 留一条记录。计划与路线见 `.design/阶段计划.md`。

## 一、M1 时期的原始清单（R01–R18）

| ID | 优先级 | 问题 | 状态 |
|---|---|---|---|
| R01 | P0 | 渲染裁剪缺失：ScrollView/TextInput/Panel 会画出 viewport | ✅ M2-E 关闭 |
| R02 | P0 | 无统一 draw list，控件内部无法按 scissor/texture 切批 | ✅ M2-E 关闭 |
| R03 | P0 | 文本仅 ASCII，FontAtlas 按 byte 索引，TextInput 只接受 ASCII | ✅ M2-D 关闭 |
| R04 | P0 | 无换行/省略号；TextInput 水平滚动与点击索引不完整 | ✅ M2-D 关闭 |
| R05 | P0 | `ex4` 手动调用 scroll/measure/arrange，绕过 UISystem | ✅ M2-F 关闭 |
| R06 | P0 | Renderer 混入 draw_frame、F11 截图、空 process_mouse_move | ✅ M2-F 关闭 |
| R07 | P1 | 全局 `active_font_atlas()` / `theme()` 影响多窗口与测试 | ✅ M3 关闭（`UIContext`） |
| R08 | P1 | 测试偏薄：缺 FontAtlas / 裁剪 / draw list / validation / ASAN | ✅ 随 M2/M3 关闭 |
| R09 | P1 | Windows 未实测，只有 `ci-windows.ps1` | 🔓 开放（暂停池） |
| R10 | P1 | 无帧时间 / draw call / 顶点数指标 | ✅ M3 关闭 |
| R11 | P1 | UI 控件直接依赖 GLFW 键码 | ✅ M3 关闭 |
| R12 | P1 | 冗余代码/资源（text.hpp、shape_maker、tester、texture.jpg、ex2、多余 shader） | ✅ M2-F / M3 关闭 |
| R13 | P1 | README 过短；SPEC / M2 任务未折叠 | ✅ M3 关闭（`docs/readme_draft.md`） |
| R14 | P2 | 单窗口 / 单 Renderer 假设 | 🔓 开放（backend 立线条件） |
| R15 | P2 | 无 text shaping：BiDi、复杂字形、字体 fallback、IME | 🔓 开放（M4.6 T10/T11 入暂停池） |
| R16 | P2 | 无 draw list / z-order / 图层 | ✅ M2-E 关闭 |
| R17 | P2 | 无序列化 / 项目模型 | 🟡 部分解决（M6.1 已提供二进制格式，资源引用待 CO-2） |
| R18 | P2 | 无安装 / 打包 | ✅ M3 关闭 |

评估归档：`docs/project_assessment.md`；审计基线：`docs/code_review.md`（历史基线）。

## 二、M3.5 之后新增的欠账

| ID | 来源 | 问题 | 影响 | 处理 |
|---|---|---|---|---|
| D01 | UI | 圆角为几何九宫格切分，边缘无抗锯齿 | 现代样式下直角可见锯齿 | 需要 MSAA 或 SDF 时再做 |
| D02 | UI | `Table`/`TreeView` 行级背景与 `MenuBar`/`TabBar` 标签不参与颜色过渡 | hover 时这些区域跳变 | 与「过渡只插值颜色」的既有决策相关，暂不动 |
| D03 | anim/ui | `SceneView` 的椭圆与文字不随 `rotation` 旋转 | 旋转椭圆/文字会有视觉错误 | 需要逐顶点旋转或字形矩阵变换 |
| D04 | ui | `Image` 仍是占位（底色 + 交叉 + 文件名） | 无法显示图片 | **CO-2 解决** |
| D05 | cosine | File 菜单的 Ctrl+N/O/S/Q 只是提示文字，未接键盘路由 | 必须点菜单 | **CO-2 顺带** |
| D06 | cosine | 覆盖已有文件不二次确认；不记忆上次目录 | 误覆盖风险；每次从 cwd 开始 | CO-2 顺带或之后 |
| D07 | cosine | 无自动保存、无崩溃恢复 | 异常退出丢改动 | 编辑器稳定后 |
| D08 | cosine | 属性面板只反映静态值，不显示动画求值结果 | 播放时看到的值与画面不一致 | 需要「轨道覆盖显示」设计 |
| D09 | examples | `ui_lab` 不进 ctest 断言 | 实验代码回归靠肉眼 | 刻意如此（实验期不设门禁） |
| D10 | ui | 逐组件单测不完整（如 `Splitter` 直到 2026-10-28 才有测试） | 回归靠 gallery 冒烟发现 | 某组件反复出问题时补 |

## 三、有意为之的取舍（不是欠账）

以下决定是权衡后**主动选择不做**，记在这里避免下次评估重新纠结：

| 编号 | 内容 | 为什么不做 |
|---|---|---|
| T01 | `ListView::draw_row` 留空，`Table::append_to` 仍自己画行 | 搬到 `draw_row` 是纯视觉重构，但当前无像素级测试；且 `snap`（像素对齐）与 `first_visible_row()`（逻辑行号）取整口径不同，合并有抖动风险。见 UI-10 |
| T02 | `ScrollView` 保留独立的二维滚动，不接 `ScrollState` | `ScrollState` 是一维的（`float offset_`）；扩成二维会让它变成什么都装的对象 |
| T03 | `Window` 标题栏不复用 `CollapsibleHeader` | 标题栏还要拖动、关闭叉、双击最大化、无标题栏整块拖动，套进去要回传事件，比各自写更绕 |
| T04 | `Layer` 已声明但未驱动命中排序 | 现有「overlay 优先 + z 降序」已工作且被 5 个测试覆盖，重写属于为一致性冒回归风险 |
| T05 | `Widget` 不拆分（406 行 / 12 种职责） | 4 个显式契约（`adopt` / `fixed_size` / `fills_parent` / `Layer` / `viewport_height`）已缓解最痛处；拆分回归面覆盖全部控件 |
| T06 | `menu_bar.hpp`（857 行）不拆 | 菜单语义（弹出、子菜单、快捷键）的合理复杂度 |
| T07 | `ui_lab` 不进 ctest | 实验代码，进 ctest 会锁死迭代速度（D09 的另一面） |
| T08 | 性能优化暂缓 | 无实测数据。先加「每帧测量次数 / batch 数 / 帧时间」统计再决定（UI-15） |
| T09 | `TextInput` 不接 `ScrollState` | 它只有一条 `clamp_scroll`，无 `ScrollMode`/平滑，收益为零 |
