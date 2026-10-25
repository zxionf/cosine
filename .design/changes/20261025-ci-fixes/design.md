# 20261025-ci-fixes

<!-- 为什么改、边界与不属于本次的范围 -->

## Linux：Slang 安装失败

**症状**：解压后 `${HOME}/tools/slang/bin/slangc` 不存在（exit 127）。

**原因**：Slang 发行包布局随版本变化。某些版本归档顶层直接是 `bin/`、`lib/`、`include/`，
用 `tar --strip-components=1` 会把 `bin/` 这一级剥掉，`slangc` 落到解压根目录。

**修法**：不再猜层级，解压后用 `find` 定位 `slangc`，并 `chmod +x`；
定位失败时打印归档前 50 个条目便于排查。

```bash
SLANGC="$(find "${HOME}/tools/slang" -maxdepth 4 -type f -name slangc | head -n 1)"
```

## Windows：找不到 Visual Studio 实例

**症状**：`cmake -G "Visual Studio 17 2022" -A x64` 报 `could not find any instance of Visual Studio`。

**修法**：不再依赖 VS 生成器的实例探测，改用：

- `ilammy/msvc-dev-cmd@v1`（arch x64）注入 MSVC 环境；
- `-G Ninja` + `-DCMAKE_BUILD_TYPE=Debug`；
- `ctest --test-dir build-ci`（Ninja 单配置，不再需要 `-C Debug`）。

同时补上 Vulkan SDK 定位：Chocolatey 安装后当前会话可能没有 `VULKAN_SDK`，
显式从 `C:\VulkanSDK\<version>` 解析并写入 `GITHUB_ENV`，供 `find_package(Vulkan)` 使用。

## 其他

- 两个 job 都新增 `Toolchain versions` 步骤，打印 CMake/Ninja/编译器版本，方便定位版本差异。
- PowerShell 步骤显式检查 `$LASTEXITCODE`，避免原生命令失败被 `pwsh` 吞掉。

## 边界

- Windows job 仍然只做构建与单元测试，不做窗口级冒烟（沿用既有约定）。
- 不引入 clang / 多编译器矩阵。
