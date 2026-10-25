# 项目评估与改进清单

> 归档日期：2026-10-24（M3-G 更新：2026-10-25）
> 代码基线：`0.3.0`（M3 完成）
> 对照文档：`.design/阶段计划.md`、`docs/index.md`
> 结论：GUI MVP 已成型并可打包；瓶颈转为 Windows 实测、动画内核与编辑器。
> 更新（2026-10-24）：R03 的 UTF-8/codepoint、C2 CJK 图集与 D3 动态字形更新已落地；R01/R02/R04/R16 的 DrawList、scissor 栈与 overlay 图层已落地；R05/R06 已随 examples/ui_demo 清理完成；E06 RenderTarget 抽象已完成。
> 构建（2026-10-24）：`cosine_text` / `cosine_ui` 去重，共享源只编译一次；CI 默认 `-j2`，测试目标关闭 module scan。
> M3（2026-10-25）：UIContext、KeyCode/Modifier、FrameStats/DebugOverlay、资源与 Vulkan 错误上下文、sanitizer + lavapipe 冒烟、0.3.0 打包与文档全部落地；14 个测试通过，ASAN 全绿。

## 1. 现状

- M1 基座稳定完成
- M2 事件 / 布局 / 控件 / 文本 / 裁剪完成
- M3 工程化完成：UIContext、输入抽象、调试指标、错误处理、CI 质量门、打包、文档
- `ctest` 18/18 通过；ASAN 通过；lavapipe 冒烟（ui_demo + ui_gallery）通过
- `build-ci` 全新构建通过
- `examples/ui_demo` 控件可交互，`--version` 输出 `cosine 0.3.0`
- 分层为 `cosine_text` / `cosine_ui` / `cosine_backend` / `cosine_app`
- 资源统一到 `assets/`，输入坐标已按 HiDPI 换算
- Linux 打包为 `dist/cosine-0.3.0-linux-x86_64.tar.gz`（约 873 KB）

## 2. 优势

1. Vulkan 基础扎实：RAII、swapchain 重建、信号量、动态缓冲、纹理、批绘制、scissor。
2. UI 方向正确：retained-mode Widget 树、measure/arrange、parent、事件冒泡、焦点、Tab、捕获、click。
3. FontAtlas 抽象方向正确：字形度量、白像素 UV、文本测量/绘制从 Renderer 抽出。
4. 布局能力较完整：Vbox/Hbox/Spacer、padding/margin、flex/grow、min/max、主轴分布、缓存、ScrollView 双轴。
5. 工程化有起色：静态库、assets、asset_path、CI 脚本、GLM/STB/slangc 声明与 fail-fast。
6. 文档与工作区：`.agent`/`.design` + sele 流程、阶段计划、决策、任务清单持续维护。

## 3. 问题归档

### P0：M2 收口前必须解决

- **R01（已解决）渲染裁剪缺失**：ScrollView、TextInput、Panel 的内容会画出 viewport；当前只有命中裁剪。
- **R02（已解决）无统一 draw list**：无法在控件内部按 scissor/texture 自动切批。
- **R03（已解决）文本仅 ASCII**：FontAtlas 按 byte 索引，TextInput 只接受 ASCII；无 UTF-8/CJK/IME。
- **R04（已解决）无换行/省略号**：Label 只支持显式 `\n`；TextInput 水平滚动不完整。
- **R05（已解决）ex4 手动 UI**：已删除 ex4，统一走 UISystem 与 examples/ui_demo。
- **R06（已解决）Renderer 混入应用逻辑**：`draw_frame()`、F11 截图、空 `process_mouse_move()` 已移除。

### P1：可维护性与可交付性

- **R07（部分解决）全局可变状态**：UIContext 已接管 UI 主题/字体；`Line`/`Rectangle`/`Triangle` 的 active atlas 仍待 graphics context。
- **R08（部分解决）测试偏薄**：18 个测试 + ASAN + lavapipe 冒烟（ui_demo/ui_gallery）；缺 Windows 实测。
- **R09（部分解决）Windows 未实测**：新增 GitHub Actions Windows job，待首次运行确认。
- **R10（已解决）性能无度量**：FrameStats + DebugOverlay 已提供 frame time/FPS/draw call/顶点/索引。
- **R11（已解决）UI 耦合 GLFW**：KeyCode/Modifier 抽象；GLFW 映射只在 Application。
- **R12（已解决）冗余代码**：已删除 `text.hpp`、`shape_maker.hpp`、`texture.jpg`、`ex2`、`ex4` 与未使用 shader。
- **R13 文档滞后**：README 过短；`docs/code_review.md` 基线较早；SPEC/M2 任务未折叠。

### P2：架构与长期

- **R14 单窗口/单 Renderer 假设**：限制多视口、离屏预览、多窗口。
- **R15 无 text shaping**：BiDi、复杂字形、字体 fallback、IME。
- **R16 无 draw list / z-order / 图层**：弹窗、Tooltip、浮层、离屏合成难做。
- **R17 无序列化/项目模型**：编辑器保存/加载/撤销无法开始。
- **R18（部分解决）无安装/打包**：Linux tar.gz 打包脚本可用；Windows zip 待实测；无安装器/签名。

## 4. 改进顺序

### 第 1 步：M2-D 文本

1. FontAtlas 改为 codepoint 维度，增加 UTF-8 解码。
2. TextInput 按 codepoint 维护光标/选择；先不做 IME。
3. measure/append 支持自动换行、最大宽度、省略号。
4. ASCII 继续用现有图集；CJK 用 FreeType + 可配置字体路径生成新图集或 SDF。
5. 修 TextInput 水平滚动后的点击索引。
6. 增加 FontAtlas / TextInput / 换行测试。

### 第 2 步：M2-E 渲染裁剪与 draw list

1. 引入 `DrawList` / `DrawCmd`。
2. 容器 push/pop scissor；ScrollView/TextInput/Panel 自动裁剪。
3. 按 texture/scissor 合批。
4. 明确 z-order，弹窗/Tooltip 置顶。
5. 保留 `WHITE` 路径，为离屏 render target 预留接口。
6. 增加 validation 冒烟。

### 第 3 步：M2-F demo 与应用层清理

1. 删除 ex4 手动 `scroll->on_scroll/measure/arrange`（已完成，ex4 已移除）。
2. ScrollView 放进 UISystem 根节点，统一走事件链。
3. 把 Renderer 的 `draw_frame`、F11 截图、空 `process_mouse_move` 移出。
4. 形成 `examples/ui_demo`；归档 `ex2`、`ex4` 与未使用资源/代码（已完成）。
5. 清理后的 demo 作为 M2 验收对象。

### 第 4 步：M3 工程化（已完成）

1. `UIContext` 替代全局 FontAtlas/Theme（M3-A）。
2. 输入抽象：KeyCode/Modifier，UI 头文件去掉 GLFW 依赖（M3-B）。
3. 帧时间/draw call/顶点数调试面板（M3-C）。
4. 资源与 Vulkan/SwapChain 错误上下文（M3-D）。
5. ASAN + lavapipe 冒烟 + Linux/Windows CI 矩阵（M3-E；Windows job 待首次远端运行）。
6. Linux/Windows 可执行目录打包（M3-F；Windows zip 待实测）。
7. 文档入口、依赖版本、README 草稿与发布说明模板（M3-G）。

### 第 4.5 步：M3.5 UI 打磨（进行中）

1. `examples/ui_gallery` 分区展示全部现有控件（Overview/Text/Inputs/Containers/Overlays/Layout）。
2. 主题切换：暗色/亮色即时生效（`UIRoot::set_theme`/`UISystem::set_theme`，gallery 按钮与 `COSINE_GALLERY_THEME`）。
3. 组件补齐：TabBar（已完成）、Radio/RadioGroup（已完成）、Progress（已完成）、Dropdown、Dialog、Table/TreeView。
4. 样式系统：圆角/密度/控件状态样式统一（主题切换已完成）。
5. 交互打磨：键盘导航、双击、滚动条、多行输入、tooltip 增强。
6. 布局补充：Grid、Wrap、百分比尺寸、显式 z-index。

### 第 5 步：M4 动画内核

1. 场景图与属性系统。
2. 时间轴、轨道、关键帧与缓动。
3. 确定性 seek 与播放器。
4. 序列化前置的稳定 ID。

## 5. 质量门

- `ctest` 全绿（当前 18 个）。
- ASAN 构建与测试通过；lavapipe 冒烟无 validation 错误。
- `ui_demo` 连续交互 60 秒，Vulkan validation 无错误。
- 项目自身代码无警告。
- `build-ci` 全新构建通过。
- `scripts/package-linux.sh` 产物可从任意 cwd 启动。

## 6. 评分（主观）

| 维度 | 分数 | 说明 |
|---|---|---|
| Vulkan 基础 | 8/10 | RenderTarget 抽象、批绘制、swapchain 重建；仍单窗口单 Renderer |
| UI 架构 | 8/10 | DrawList/裁剪/overlay/UIContext 完成；缺多窗口与 IME |
| 文本 | 7.5/10 | UTF-8 + CJK + 换行/省略；无 shaping 与 SDF |
| 测试 | 7.5/10 | 14 个测试 + ASAN + lavapipe；缺渲染像素比对 |
| 跨平台 | 5.5/10 | Linux 已验证并可打包；Windows 脚本就绪未实测 |
| 文档/流程 | 8/10 | docs/index、ci、packaging、dependencies 齐全；README 待作者定稿 |
| MVP 完成度 | 约 85% | GUI 框架可交付；安装器与 Windows 验证待补 |
