# 历史与已完成基线

> 本文承接 `.design/阶段计划.md` 迁出的历史内容：已完成基线（M0 之前的骨架阶段）、
> 里程碑历史摘要。**路线与计划**请看 `.design/阶段计划.md`；
> **当前设计事实**请看 `.design/SPEC.md`。历史细节最终以 git 为准。

## 1. 已完成基线

以下内容已经存在于仓库中，作为后续路线的起点。

### 1.1 XEL 解释器

提交区间：`31a1a1a` → `3b72f8c`，以及 `91b7f96`。

- 词法：`token` / `lexer`，支持标识符、数字、字符串、运算符。
- 语法：递归下降 `parser`，生成 AST；`ast/expr`、`ast/stmt` 分离。
- 求值：`evaluator` + `environment`，支持四则运算、比较、变量、作用域。
- 控制流：分支、循环；修复过循环条件永真、`env.get()` 递归等问题。
- 函数：声明、调用、返回值、native 函数。
- 工具：`xel-vs-extension` VS Code 语法扩展。
- 测试：`test_interpreter_token`、`test_intepreter_tree_structure`、`test_intepreter_tree_evaluator`。

### 1.2 Vulkan 后端

提交区间：`34963ad` → `dc69539`，以及后续修复。

- 实例、物理设备、逻辑设备、队列、Surface。
- SwapChain 创建、重建、动态视口、呈现模式选择。
- 命令池、命令缓冲区、单次命令。
- 图形管线、顶点/片元着色器、slang 编译接入。
- Push constant、2D 变换（scale / translate）。
- Staging buffer、device-local 顶点/索引缓冲、动态顶点缓冲。
- Uniform buffer、描述符集/描述符池、组合图像采样器。
- 纹理加载、采样、透明窗口。
- 批量绘制、索引绘制、scissor 裁剪。
- 帧生命周期拆分：`begin()` / `end()`、`MAX_FRAMES_IN_FLIGHT`。
- F11 截图到 PNG。
- 信号量/围栏修复；`5122c15` 修复 acquire 返回 out-of-date 时使用未初始化 image index 导致的越界崩溃。

### 1.3 图形与 UI 骨架

提交区间：`65e0cc3` → `87fc4e1`。

- `graphics/`：`Vertex`、`Drawable`、`DrawBatch`、`Rectangle`、`Triangle`、`Line`、`ShapeMaker`、`Text`。
- `ui/`：`Widget`、`Rect`、`Align`、`ColorRect`、`Vbox`、`UIRoot`、`ScrollView`、`UISystem`。
- 布局：measure / arrange / local-global bounds / align / padding 雏形。
- 事件：鼠标移动、滚轮、hover / pressed 雏形。
- 演示：`ex4` 为当时 UI 主 demo（已归档为 `examples/ui_demo`）；`ex2` 为文本 demo（已删除）。

### 1.4 近期修复与清理

- `5122c15`：修复 swapchain out-of-date 后继续 present 旧图像、每图信号量未重建的问题。
- `2fcdcbf` / `ebcb86d`：删除 imgui、ImGuiColorTextEdit、`movy/`、`test/testimgui.cpp`、`test/testdocking.cpp`，同步 CMake 与 `.gitignore`。
- `09e3953`：加入 `docs/code_review.md` 审计。
- `02f0302`：引入 `.agent/` / `.design/` 工作区与 sele 流程。
- `20261024-m1-stability`：M1 完成——P0/P1 修复、backend 静态库、`assets/` 资源定位、CTest/CI 脚本；SPEC 已折叠。

### 1.5 当前已知缺口（2026-10-26 更新）

M1/M2 已清理 `ex0` / `ex1` / `ex3` / `font_renderer.*` 等历史遗留，并补齐布局、事件、文本、裁剪测试与 CI。
当前开放缺口见 `.design/阶段计划.md` 的「技术债清单」与「风险」两节；不再在此重复。

## 2. 历史变更摘要

| 时间线 | 内容 | 代表提交 |
|---|---|---|
| 解释器 | token/lexer/parser/AST/evaluator/env/控制流/函数/native | `31a1a1a` → `3b72f8c` |
| Vulkan 基础 | 实例、设备、交换链、命令、管线、重建 | `34963ad` → `1a8bd92` |
| 渲染能力 | push constant、2D 变换、staging、index、uniform、纹理、文字、截图 | `9df162d` → `339963c` |
| 动态与批处理 | 动态顶点、帧生命周期、描述符、批绘制、裁剪 | `477010d` → `dd619fe` |
| UI 骨架 | Widget、Vbox、对齐、UI 系统、事件、ScrollView | `4d19445` → `87fc4e1` |
| 审计与修复 | code_review、swapchain 越界修复 | `09e3953`、`5122c15` |
| 清理与工作区 | 删除 imgui/movy、引入 `.agent`/`.design` | `2fcdcbf`、`ebcb86d`、`02f0302` |

M1 之后的阶段（M2 起）不再按提交记录，改为按阶段归档：
完成细节见 `.design/SPEC.md` 对应章节，执行过程见 `.agent/work/<id>/`。
