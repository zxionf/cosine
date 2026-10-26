# 第三方依赖

所有依赖通过 CMake `FetchContent` 获取，版本在根 `CMakeLists.txt` 中锁定。
升级时改对应 `GIT_TAG`，然后重新配置构建目录。

## 构建期依赖

| 依赖 | 版本 / 提交 | 用途 | 获取方式 |
|---|---|---|---|
| Vulkan | 系统 `find_package(Vulkan)`，API 1.4 | 图形 API 与 `vulkan_raii.hpp` | 系统安装（Vulkan SDK / `libvulkan-dev`） |
| GLFW | `3.4` | 窗口、输入、surface 创建 | FetchContent（git） |
| FreeType | `VER-2-13-3` | TTF 字形栅格化，生成字体图集 | FetchContent（git） |
| stb | `003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5` | `stb_image`（图集加载）、`stb_image_write` | FetchContent（git，固定提交） |
| GLM | `1.0.1` | 向量/矩阵 | FetchContent（git） |
| Slang | `2026.17.1`（CI 固定） | 编译 `ui.slang` 为 SPIR-V | 系统安装或 `-DSLANGC_EXECUTABLE=` |
| glslc | 随 Vulkan SDK | 备用 GLSL 着色器编译 | Vulkan SDK |

## 运行期依赖

Linux 产物动态链接：

```text
libvulkan.so.1   （显卡驱动提供）
libstdc++.so.6
libm.so.6
libgcc_s.so.1
libc.so.6
```

GLFW、FreeType、GLM 静态链接进可执行文件。
Wayland/X11 支持由 GLFW 编译期选项决定：

```bash
-DCOSINE_GLFW_WAYLAND=ON    # 默认
-DCOSINE_GLFW_X11=OFF       # 默认
```

Windows 依赖系统 Vulkan 运行时（显卡驱动）与 MSVC 运行库。

## 字体

运行期字体来源不在依赖链里：

1. `COSINE_CJK_FONT` 指定的 TTF/TTC
2. 系统字体（Noto CJK、Source Han Sans、微软雅黑等）

打包不包含 CJK 字体，详见 `packaging.md`。

## 升级流程

1. 修改根 `CMakeLists.txt` 的 `GIT_TAG`
2. 删除对应 `build/_deps/<dep>-src` 或使用全新构建目录
3. `./scripts/ci-linux.sh` 全量构建 + 测试
4. `./scripts/smoke-lavapipe.sh` 冒烟
5. 更新本文件的版本表

Slang 升级时同步更新 `docs/ci.md` 记录的工具链前提；若已重建远端 CI，再同步工作流中的 `SLANG_VERSION`。

## 平台前提

- Linux：Vulkan loader + 至少一个 ICD（硬件驱动或 lavapipe）
- Windows：Vulkan SDK（构建期）、显卡驱动（运行期）
- 字体：目标机器需有 CJK 字体，或用 `COSINE_CJK_FONT` 指定
