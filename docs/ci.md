# 构建与 CI

## 本地命令

```bash
# 常规构建 + 测试（复用 build/_deps，避免重复 clone 依赖）
./scripts/ci-linux.sh

# 自定义并行度 / 构建目录
COSINE_JOBS=8 COSINE_BUILD_DIR=build-local ./scripts/ci-linux.sh

# AddressSanitizer 构建 + 测试
COSINE_SANITIZE=address COSINE_BUILD_DIR=build-asan ./scripts/ci-linux.sh
```

`COSINE_SANITIZE` 支持 `address`、`undefined`、`thread`（可用分号组合）。
它只作用于本项目 target，不重新编译第三方依赖。

## lavapipe 冒烟

`scripts/smoke-lavapipe.sh` 用 Mesa 软件 Vulkan 驱动依次运行 `ui_demo` 与 `ui_gallery`，
并检查日志中不出现 Vulkan validation 错误。

```bash
# 系统已安装 lavapipe（mesa-vulkan-drivers 提供 lvp_icd.*.json）
./scripts/smoke-lavapipe.sh

# 指定 ICD 或裸 .so 所在目录
COSINE_LAVAPIPE_ICD=/path/lvp_icd.json ./scripts/smoke-lavapipe.sh
COSINE_LAVAPIPE_LIB_DIR=/path/to/lib ./scripts/smoke-lavapipe.sh

# 调整时长
COSINE_SMOKE_SECONDS=30 ./scripts/smoke-lavapipe.sh
```

没有可用的 ICD 或没有显示环境（`WAYLAND_DISPLAY`/`DISPLAY`）时脚本会 skip 并返回 0。

## GitHub Actions

`.github/workflows/ci.yml` 包含两个 job：

- **Linux**：GCC + Ninja，X11 版 GLFW，`ctest`，ASAN 构建与测试，`xvfb-run` + lavapipe 冒烟。
- **Windows**：MSVC（`ilammy/msvc-dev-cmd` + Ninja），Chocolatey 安装 Vulkan SDK，构建与 `ctest`。

两个 job 都通过仓库脚本或等价命令构建，避免 CI 与本地命令分叉。
Slang 版本固定在工作流顶部的 `SLANG_VERSION`，升级时同步更新。

### Slang 安装

不同版本的发行包布局并不一致（有的顶层直接是 `bin/`，有的多一层目录），
因此工作流不使用 `--strip-components`，而是解压后 `find` 定位 `slangc`：

```bash
SLANGC="$(find "${HOME}/tools/slang" -maxdepth 4 -type f -name slangc | head -n 1)"
```

找不到时会打印归档前 50 个条目再失败，便于定位版本差异。

### Windows 构建器

- 不使用 `-G "Visual Studio 17 2022"`（镜像上 VS 实例探测不稳定），
  改为 `ilammy/msvc-dev-cmd@v1` 设置 MSVC 环境 + `-G Ninja`。
- `find_package(Vulkan)` 依赖 `VULKAN_SDK`：Chocolatey 安装后当前会话未必有该变量，
  工作流会显式解析 `C:\VulkanSDK\<version>` 并写入 `GITHUB_ENV`。

## 已知限制

- Windows job 只做构建与单元测试，没有窗口级运行冒烟。
- 工作流要求 CMake >= 4.0.0（项目 `cmake_minimum_required`），镜像自带的 CMake 版本需满足。
- Slang 归档布局随版本变化，定位逻辑在 `docs/ci.md` 有说明。
- lavapipe 冒烟只验证启动与 validation 干净，不做像素比对。
- 未启用 clang / 多编译器矩阵。
