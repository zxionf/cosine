# 文档索引

cosine 是一个 C++20 + Vulkan 的 2D GUI 与动画引擎项目，XEL 是它的脚本语言。
本页是文档入口；根目录 README 由作者维护，本目录只放工程文档。

## 阅读顺序

1. `../.design/SPEC.md` — 当前设计事实（唯一真相源）
2. `todo.md` — 待办：欠账、暂停池、风险、未决问题、有意不做的决策
3. `plan_archive.md` — 已完成阶段的计划原文归档
4. `project_format.md` — `.cosine` 项目文件格式
5. `examples.md` — 示例用途、按键、环境变量
6. `ui_architecture.md` — UI 层架构
7. 按需深入具体主题（见下表）

## 文档地图

| 文档 | 内容 |
|---|---|
| `index.md` | 本页：入口与索引 |
| `todo.md` | 待办：欠账（R/D）、暂停池、风险、未决问题、有意不做的决策 |
| `ui_architecture.md` | UI 层架构：三层节点、布局契约、层级排序、模态、缩放、生命周期、契约断言 |
| `ui_theme_tokens.md` | 主题令牌 ↔ 样式角色对照表（12 个角色） |
| `todo-ui.md` | UI 层改造进度与剩余批次（R / Q 表） |
| `examples.md` | 示例用途、按键、环境变量（含 `ui_lab` 实验室） |
| `ci.md` | 构建命令、sanitizer、冒烟、测试分层与质量门、远端 CI 现状 |
| `dependencies.md` | 第三方依赖、版本锁定、升级方式 |
| `packaging.md` | 版本号、Linux/Windows 打包、资源与字体策略、运行依赖 |
| `project_format.md` | `.cosine` 项目文件格式 |
| `font_atlas.md` | 字体图集格式、FreeType 生成、动态字形、DPI |
| `history.md` | 已完成基线（M0 之前的骨架阶段）与历史变更摘要 |
| `plan_archive.md` | 已完成阶段（M1–M6.4b）的计划原文归档 |
| `interpreter.md` | XEL 解释器结构与实现笔记 |
| `release_notes_template.md` | 发布说明模板 |
| `readme_draft.md` | README 建议稿（未应用，供作者定稿） |
| `project_assessment.md` | 项目评估与改进清单（**历史归档**，结论停在 0.3.0） |
| `sele-report.md` | sele 0.21.4 在 Windows 上的 bug 报告（上游反馈稿） |
| `sele-0.21.3-agent.md` | sele 执行 agent 操作手册 |
| `review/` | 评估与审计归档（只读，不再改动） |

## 代码结构速查

```text
src/
├── anim/             缓动、场景图、属性、时间轴、播放器、命令栈（只依赖 glm）
├── backend/          窗口、Vulkan 上下文、swapchain、renderer、资源路径
│   └── vulkan/       RenderTarget、Renderer（DrawList 消费端）
├── events/           InputEvent、KeyCode/Modifier
├── graphics/         DrawList/Drawable、图元、FontAtlas、FrameStats
├── ui/               UI 层（按职责分目录）
│   ├── geom/         纯几何与尺寸分配（Rect / Align / LayoutBudget）
│   ├── layout/       布局意图 / 工具 / 测量缓存
│   ├── style/        主题 / 样式角色 / 过渡槽 / 绘制部件
│   ├── input/        Interaction（交互状态唯一所有者）
│   ├── semantics/    语义投影（WidgetRole / Semantics）
│   ├── node/         Node / LayoutNode / Widget
│   ├── system/       UIRoot / UISystem / UIContext
│   └── core/ containers/ composites/ views/    控件（按复杂度分四级）
├── cosine/           编辑器产品层（cosine_editor + cosine 可执行）
└── xel_application.* 窗口 + Vulkan + UI 的装配与事件桥接

examples/
├── ui_demo/          文字排版实验台
├── ui_gallery/       控件与样式验收展厅
├── ui_anim_demo/     最小动画演示
├── ui_anim_showcase/ 动画能力全景
├── ui_lab/           高级 UI 实验室（实验代码，不进 ctest）
├── moss/             最小渲染 demo
└── font_lab/         字体图集实验

test/                 CTest 用例（81 个，全部在 Release 下也真实执行）
interpreter/          XEL 解释器（与 UI 解耦，可嵌入）
scripts/              CI、打包、冒烟脚本
assets/               shader、字体与图集
```

## 快速开始

```bash
# 构建 + 测试
./scripts/ci-linux.sh

# 运行示例
./build/examples/ui_demo        # 文字实验台，F1–F5 切调试层
./build/examples/ui_gallery     # 控件展厅，F1 调试面板
./build/examples/ui_anim_demo   # 最小动画

# 运行编辑器（产品）
./build/src/cosine/cosine

# 冒烟（正常退出）
(cd build/examples && COSINE_EXIT_AFTER=3 ./ui_gallery)

# 打包
./scripts/package-linux.sh
```

各示例的按键与环境变量见 `examples.md`；详细命令见 `ci.md` 与 `packaging.md`。
