# 文档索引

cosine 是一个 C++20 + Vulkan 的 2D GUI 与动画引擎项目，XEL 是它的脚本语言。
本页是文档入口；根目录 README 由作者维护，本目录只放工程文档。

## 阅读顺序

1. `../.design/阶段计划.md` — 里程碑与验收标准（M1–M8）
2. `../.design/SPEC.md` — 当前有效的设计事实
3. `project_assessment.md` — 现状评估、问题清单与改进顺序
4. 按需深入具体主题（见下表）

## 文档地图

| 文档 | 内容 |
|---|---|
| `index.md` | 本页：入口与索引 |
| `dependencies.md` | 第三方依赖、版本锁定、升级方式 |
| `ci.md` | 构建命令、sanitizer、lavapipe 冒烟、CI 矩阵 |
| `packaging.md` | 版本号、Linux/Windows 打包、资源与字体策略、运行依赖 |
| `font_atlas.md` | 字体图集格式、FreeType 生成、动态字形、字号策略 |
| `interpreter.md` | XEL 解释器结构与实现笔记 |
| `note1.md` | 开发过程笔记 |
| `release_notes_template.md` | 发布说明模板 |
| `readme_draft.md` | README 建议稿（未应用，供作者定稿） |
| `code_review.md` | 历史代码评审记录（基线较早，保留参考） |
| `project_assessment.md` | 项目评估与改进清单 |

## 代码结构速查

```text
src/
├── backend/          窗口、Vulkan 上下文、swapchain、renderer、资源路径
│   └── vulkan/       RenderTarget、Renderer（DrawList 消费端）
├── events/           InputEvent、KeyCode/Modifier
├── graphics/         DrawList/Drawable、图元、FontAtlas、FrameStats
├── ui/               Widget 树、布局、控件、UISystem、UIContext、DebugOverlay
└── xel_application.* 窗口 + Vulkan + UI 的装配与事件桥接

examples/ui_demo/     文字实验台（排版角色、对齐、换行、行距、CJK、文字调试层）
examples/ui_gallery/  UI 打磨验收 demo（分区展示全部控件与交互）
test/                 CTest 用例（布局/事件/控件/文本/渲染/打包相关）
interpreter/          XEL 解释器（与 UI 解耦，后续作为可嵌入库）
scripts/              CI、打包、冒烟脚本
```

## 快速开始

```bash
# 构建 + 测试
./scripts/ci-linux.sh

# 运行 demo（最小面板）
./build-ci/examples/ui_demo

# 运行 UI gallery（全部控件分区展示，F1 开关调试面板）
./build-ci/examples/ui_gallery

# 打包
./scripts/package-linux.sh
```

详细命令见 `ci.md` 与 `packaging.md`。

## 当前状态（0.3.0）

- GUI MVP：控件集、事件与焦点、布局、UTF-8+CJK 文本、DrawList 裁剪与 overlay、调试面板。
- UI gallery：`examples/ui_gallery` 分区展示全部控件与交互，支持暗色/亮色主题即时切换（M3.5 起点）。
- 文字实验台：`examples/ui_demo` 用 F1–F5 切换文字调试层（字形盒/行框/基线/advance）与像素对齐，验证 `Label` 对齐、换行与行距。
- 文字质量：字形按绘制字号逐档栅格化（不再放大 16px 基准图集），降部与细描边保持清晰。
- 新增控件：`TabBar`/`TabView`、`Radio`/`RadioGroup`、`Progress`、`Dropdown`、`Dialog`、`MenuBar`；`ScrollView` 可见滚动条；`ScrollPanel` 可滚动面板；`TextInput` 支持多行。
- 工程化：静态库分层、CMake 去重、资源解析错误、sanitizer、lavapipe 冒烟、打包。
- 未完成：Windows 实测、CI 首次远端运行、安装器；动画内核与编辑器属于 M4–M6。
