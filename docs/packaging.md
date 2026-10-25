# 打包

MVP 的发布形态是**免安装的压缩包**：解压后直接运行，不需要系统级安装。

## 版本号

版本号唯一来源是根 `CMakeLists.txt` 的 `project(cosvideo VERSION ...)`。
CMake 会生成 `xel/version.hpp`，`ui_demo --version` 输出：

```text
cosine 0.3.0
```

打包脚本从 `CMakeCache.txt` 读取 `CMAKE_PROJECT_VERSION`，不在脚本里重复维护版本。

## Linux

```bash
./scripts/package-linux.sh
```

产物：

```text
dist/cosine-<version>-linux-<arch>/
├── ui_demo
├── assets/
│   ├── fonts/default8.png
│   ├── fonts/gb2312_level1.txt
│   └── shaders/ui.slang.spv
├── run.sh
├── VERSION
└── README.md

dist/cosine-<version>-linux-<arch>.tar.gz
```

环境变量：

| 变量 | 作用 |
|---|---|
| `COSINE_PACKAGE_BUILD_DIR` | 构建目录（默认 `build-release`） |
| `COSINE_PACKAGE_OUT_DIR` | 输出目录（默认 `dist`） |
| `COSINE_JOBS` | 并行度（默认 2） |
| `COSINE_SKIP_STRIP=1` | 跳过 `strip` |

脚本会用 Release 构建、`strip` 二进制，并在打包后执行 `--version` 做一次冒烟。

## Windows

```powershell
./scripts/package-windows.ps1
```

产物：

```text
dist\cosine-<version>-windows-x64\
├── ui_demo.exe
├── assets\
├── run.cmd
├── VERSION
└── README.md

dist\cosine-<version>-windows-x64.zip
```

MSVC 不提供 `strip`；Release 产物已不含调试信息。

## 资源解析顺序

运行期按以下顺序查找 `assets/`（见 `src/backend/asset_path.cpp`）：

1. `COSINE_ASSET_DIR` 环境变量
2. 可执行文件旁的 `assets/`
3. 构建期注入的 `COSINE_ASSET_DIR`

因此打包目录必须保持 `ui_demo` 与 `assets/` 同级；从任意 cwd 启动都能找到资源。

## 字体策略

打包内置：

- `assets/fonts/default8.png`：ASCII 位图回退
- `assets/fonts/gb2312_level1.txt`：GB2312 一级常用字覆盖表

CJK 字形默认从系统字体生成图集，查找顺序见 `graphics::find_cjk_font()`：

1. `COSINE_CJK_FONT`（可用 `COSINE_CJK_FONT_FACE` 指定 ttc 索引）
2. Linux：Noto CJK、Source Han Sans、文泉驿
3. Windows：微软雅黑、黑体、宋体

**不内置 CJK TTF**：字体体积大且许可证各异。若目标机器没有中文字体，可以：

```bash
COSINE_CJK_FONT=/path/NotoSansCJK-Regular.ttc ./ui_demo
```

或把字体放进 `assets/fonts/` 并自行设置上面的环境变量。

## 运行依赖

Linux 产物动态依赖：

```text
libvulkan.so.1
libstdc++.so.6
libm.so.6
libgcc_s.so.1
libc.so.6
```

GLFW、FreeType、GLM 均为静态链接；Wayland/X11 由 GLFW 在编译期决定。
Windows 产物依赖系统 Vulkan 运行时（显卡驱动提供）与 MSVC 运行库。

## 后续

- 安装器（deb/msi）、代码签名、自动更新不在 MVP 范围。
- Windows 产物的首次完整流程验证依赖真实 Windows 环境或 CI。
