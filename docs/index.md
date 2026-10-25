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

examples/ui_demo/     MVP 验收 demo（面板、控件、滚动、CJK、调试面板）
test/                 CTest 用例（布局/事件/控件/文本/渲染/打包相关）
interpreter/          XEL 解释器（与 UI 解耦，后续作为可嵌入库）
scripts/              CI、打包、冒烟脚本
```

## 快速开始

```bash
# 构建 + 测试
./scripts/ci-linux.sh

# 运行 demo
./build-ci/examples/ui_demo

# 打包
./scripts/package-linux.sh
```

详细命令见 `ci.md` 与 `packaging.md`。

## 当前状态（0.3.0）

- GUI MVP：控件集、事件与焦点、布局、UTF-8+CJK 文本、DrawList 裁剪与 overlay、调试面板。
- 工程化：静态库分层、CMake 去重、资源解析错误、sanitizer、lavapipe 冒烟、打包。
- 未完成：Windows 实测、CI 首次远端运行、安装器；动画内核与编辑器属于 M4–M6。
