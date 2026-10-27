# 文档索引

cosine 是一个 C++20 + Vulkan 的 2D GUI 与动画引擎项目，XEL 是它的脚本语言。
本页是文档入口；根目录 README 由作者维护，本目录只放工程文档。

## 阅读顺序

1. `../.design/阶段计划.md` — 路线、阶段目标与出口条件（M1–M8）
2. `../.design/SPEC.md` — 当前有效的设计事实（唯一真相源）
3. `examples.md` — 四个示例的用途、按键与专属环境变量
4. `project_assessment.md` — 历史评估与改进清单（归档）
5. 按需深入具体主题（见下表）

## 文档地图

| 文档 | 内容 |
|---|---|
| `index.md` | 本页：入口与索引 |
| `examples.md` | 四个示例的用途、按键、环境变量影响 |
| `dependencies.md` | 第三方依赖、版本锁定、升级方式 |
| `ci.md` | 构建命令、sanitizer、冒烟、测试分层与质量门、远端 CI 现状 |
| `packaging.md` | 版本号、Linux/Windows 打包、资源与字体策略、运行依赖 |
| `font_atlas.md` | 字体图集格式、FreeType 生成、动态字形、行高、脏矩形上传、DPI |
| `history.md` | 已完成基线（M0 之前的骨架阶段）与历史变更摘要 |
| `interpreter.md` | XEL 解释器结构与实现笔记 |
| `note1.md` | 开发过程笔记 |
| `release_notes_template.md` | 发布说明模板 |
| `readme_draft.md` | README 建议稿（未应用，供作者定稿） |
| `code_review.md` | 历史代码评审记录（基线较早，保留参考） |
| `project_assessment.md` | 项目评估与改进清单（历史归档） |

## 代码结构速查

```text
src/
├── anim/             缓动、场景图、属性、时间轴、播放器（只依赖 glm）
├── backend/          窗口、Vulkan 上下文、swapchain、renderer、资源路径
│   └── vulkan/       RenderTarget、Renderer（DrawList 消费端）
├── events/           InputEvent、KeyCode/Modifier
├── graphics/         DrawList/Drawable、图元、FontAtlas、FrameStats
├── ui/               Widget 树、布局、控件、UISystem、UIContext、SceneView、DebugOverlay
└── xel_application.* 窗口 + Vulkan + UI 的装配与事件桥接

examples/ui_demo/           XEL Text Lab：文字排版实验台
examples/ui_gallery/        XEL UI Gallery：控件与样式验收展厅
examples/ui_anim_demo/      XEL Animation Demo：最小动画演示
examples/ui_anim_showcase/  XEL Animation Showcase：动画能力全景
test/                       CTest 用例（51 个）
interpreter/                XEL 解释器（与 UI 解耦，M5 起作为可嵌入库）
scripts/                    CI、打包、冒烟脚本
assets/                     shader、字体与图集
```

## 快速开始

```bash
# 构建 + 测试
./scripts/ci-linux.sh

# 运行示例
./build/examples/ui_demo        # 文字实验台，F1–F5 切调试层
./build/examples/ui_gallery     # 控件展厅，F1 调试面板
./build/examples/ui_anim_demo   # 最小动画

# 冒烟（正常退出）
(cd build/examples && COSINE_EXIT_AFTER=3 ./ui_gallery)

# 打包
./scripts/package-linux.sh
```

各示例的按键与环境变量见 `examples.md`；详细命令见 `ci.md` 与 `packaging.md`。

## 当前状态（0.4.0）

- **GUI 框架**：Widget 树与 measure/arrange；Vbox/Hbox/Grid/Wrap、百分比尺寸、宽高比、
  绝对定位与 z-index；TabBar/TabView、Radio/RadioGroup、Progress、Dropdown、Dialog、MenuBar、
  Table、TreeView、Image（占位）、Tooltip；样式系统（主题/经典样式/状态过渡/焦点策略/圆角）；
  指针手势、键盘导航、拖拽反馈；Smooth/Fast/Line 三档滚动。
- **文本**：UTF-8 + CJK、FreeType 运行时图集、逐字号栅格化、动态字形、行高语义、测量缓存、
  脏矩形上传、DPI 缩放；`ui_demo` 可开字形盒/行框/基线/advance 调试层。
- **动画内核（M4）**：缓动、场景图、属性系统、时间轴与关键帧、播放器与时间点事件、
  `SceneView` 渲染；`ui_anim_demo` 与 `ui_anim_showcase` 覆盖最小演示与能力全景。
- **工程化**：静态库分层、51 个 CTest 用例、ASAN、lavapipe 冒烟、Linux/Windows 打包脚本。
- **未完成**：Windows 实测、IME 与文字 shaping、SDF/MSDF、序列化；
  下一步是 M6.0（离屏渲染与 PNG 导出），XEL 绑定（M5）排在编辑器之后。
