# 项目评估与改进清单

> 归档日期：2026-10-24
> 代码基线：`e527b44`（M2-C 完成）
> 对照文档：`.design/阶段计划.md`、`.agent/work/20261024-m2-gui-mvp/tasks.md`
> 结论：Vulkan/UI 骨架与交互链路已成型；当前瓶颈是文本、裁剪和 demo/工程化，不是 Vulkan。
> 更新（2026-10-24）：R03 的 UTF-8/codepoint、C2 CJK 图集与 D3 动态字形更新已落地；R01/R02/R04/R16 的 DrawList、scissor 栈与 overlay 图层已落地；R05/R06 已随 examples/ui_demo 清理完成；E06 RenderTarget 抽象已完成。
> 构建（2026-10-24）：`cosine_text` / `cosine_ui` 去重，共享源只编译一次；CI 默认 `-j2`，测试目标关闭 module scan。

## 1. 现状

- M1 基座稳定完成
- M2-A 事件完成
- M2-B 布局完成
- M2-C 控件集完成
- `ctest` 9/9 通过
- `build-ci` 全新构建通过
- `examples/ui_demo` 控件可交互
- backend 已抽成 `cosine_backend` 静态库
- 资源统一到 `assets/`，输入坐标已按 HiDPI 换算

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
- **R08 测试偏薄**：10 个测试；缺 ASAN、lavapipe 渲染冒烟和 Windows 实测。
- **R09 Windows 未实测**：只有 `ci-windows.ps1`，无实际构建结果。
- **R10 性能无度量**：每帧分配、逐批绑定、无 draw call/顶点/帧时间面板。
- **R11 UI 耦合 GLFW**：控件直接使用 GLFW 键码。
- **R12（已解决）冗余代码**：已删除 `text.hpp`、`shape_maker.hpp`、`texture.jpg`、`ex2`、`ex4` 与未使用 shader。
- **R13 文档滞后**：README 过短；`docs/code_review.md` 基线较早；SPEC/M2 任务未折叠。

### P2：架构与长期

- **R14 单窗口/单 Renderer 假设**：限制多视口、离屏预览、多窗口。
- **R15 无 text shaping**：BiDi、复杂字形、字体 fallback、IME。
- **R16 无 draw list / z-order / 图层**：弹窗、Tooltip、浮层、离屏合成难做。
- **R17 无序列化/项目模型**：编辑器保存/加载/撤销无法开始。
- **R18 无安装/打包**：离“可交付 GUI MVP”还有距离。

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

### 第 4 步：M3 工程化

1. `UIContext` 替代全局 FontAtlas/Theme。
2. Linux + Windows CI 真正跑起来。
3. lavapipe 渲染冒烟 + ASAN。
4. 帧时间/draw call/顶点数调试面板。
5. README、依赖版本、资源格式文档。
6. Linux/Windows 可执行目录打包。

## 5. 质量门

- `ctest` 全绿，且新增文本/裁剪/draw list 测试。
- `ui_demo` 连续交互 60 秒，Vulkan validation 无错误。
- 项目自身代码无警告。
- `build-ci` 全新构建通过。
- M2 收口前不进入 M4 动画内核。

## 6. 评分（主观）

| 维度 | 分数 | 说明 |
|---|---|---|
| Vulkan 基础 | 7.5/10 | 能跑、能批绘制，仍有应用逻辑混入 |
| UI 架构 | 7/10 | 事件/布局/焦点闭环，缺 draw list 与裁剪 |
| 文本 | 4/10 | ASCII 可用，UTF-8/CJK/换行/省略号缺失 |
| 测试 | 5/10 | 6 个测试，缺渲染与 ASAN |
| 跨平台 | 4/10 | Linux 验证，Windows 未实测 |
| 文档/流程 | 6/10 | sele 流程好，README 与审计文档滞后 |
| MVP 完成度 | 约 55% | 交互骨架完成，距离可交付 GUI 框架还差文本、裁剪、工程化 |
