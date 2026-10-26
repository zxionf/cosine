# cosine animation engine

cosine 是一个用 C++20 + Vulkan 编写的 2D GUI 与动画引擎。
它不依赖第三方 UI 库，控件、布局、文本与绘制批处理都是自带实现。

XEL 是 cosine 的脚本语言，用于描述动画与场景；解释器目前在 `interpreter/`，
后续会作为可嵌入库接入动画内核。

## 当前状态

版本 `0.3.0`，处于 **GUI MVP** 阶段：

- Retained-mode Widget 树：measure/arrange、padding/margin、flex/grow、主轴分布
- 事件：命中路径、冒泡与消费、鼠标捕获、焦点与 Tab 遍历
- 控件：Panel、Label、Button、Checkbox、Slider、TextInput、ScrollView/List、Splitter、Tooltip
- 文本：UTF-8 + 常用 CJK，自动换行/省略号，FreeType 运行时图集与动态字形
- 渲染：DrawList 批处理、scissor 裁剪栈、overlay 图层、RenderTarget 抽象
- 调试：FPS / frame time / draw call / 顶点索引 / hover-focus 面板

动画内核（M4）、XEL 绑定（M5）、时间轴编辑器（M6）尚未开始。

## 构建

需要 CMake ≥ 4.0、C++20 编译器、Vulkan SDK（含 `glslc`）和 Slang（`slangc`）。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

或者直接用脚本：

```bash
./scripts/ci-linux.sh            # 构建 + 测试
COSINE_SANITIZE=address \
  COSINE_BUILD_DIR=build-asan ./scripts/ci-linux.sh   # ASAN
```

`slangc` 不在 PATH 时：

```bash
cmake -S . -B build -DSLANGC_EXECUTABLE=/path/to/slangc
```

## 运行 demo

```bash
./build/examples/ui_demo
```

- `F1` 开关调试面板
- `COSINE_DEBUG_OVERLAY=1` 启动即显示
- `--version` 打印 `cosine <version>`

## 字体

默认使用系统 CJK 字体生成图集；没有可用字体时回退到 `assets/fonts/default8.png`。
可以显式指定：

```bash
COSINE_CJK_FONT=/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc \
COSINE_CJK_FONT_FACE=2 ./build/examples/ui_demo
```

## 资源路径

`assets/` 按以下顺序查找：

1. `COSINE_ASSET_DIR`
2. 可执行文件旁的 `assets/`
3. 构建期注入的目录（构建目录内运行时）

## 打包

```bash
./scripts/package-linux.sh     # dist/cosine-<version>-linux-<arch>.tar.gz
./scripts/package-windows.ps1  # dist/cosine-<version>-windows-x64.zip
```

## 文档

工程文档入口：`docs/index.md`。

- 构建与 CI：`docs/ci.md`
- 打包与字体策略：`docs/packaging.md`
- 依赖版本：`docs/dependencies.md`
- 字体图集：`docs/font_atlas.md`
- 现状评估：`docs/project_assessment.md`

## 已知限制

- Windows 构建脚本已就绪，但尚未在真实环境完整验证
- 无 IME、无文字 shaping（BiDi、复杂字形）
- 无安装器、代码签名与自动更新
- 2.5D/3D、动画内核与编辑器仍在路线图中
