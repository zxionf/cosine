# 20261025-ci-fixes handover

## 两个失败

### 1. Linux：`slangc: No such file or directory`

`tar -xzf slang.tar.gz -C ~/tools/slang --strip-components=1` 假设归档里有一层 wrapper 目录。
Slang 某些版本的归档顶层直接是 `bin/`、`lib/`、`include/`，strip 一层后 `bin/slangc` 变成 `slangc`。

修法：解压不 strip，`find` 定位后 `chmod +x`，找不到时打印归档结构。

### 2. Windows：`could not find any instance of Visual Studio`

`-G "Visual Studio 17 2022"` 依赖 VS 实例探测，在 runner 上失败。

修法：

- `ilammy/msvc-dev-cmd@v1`（arch x64）设置 MSVC 环境
- `-G Ninja -DCMAKE_BUILD_TYPE=Debug`
- `ctest --test-dir build-ci`（单配置，无需 `-C Debug`）
- 追加 `VULKAN_SDK` 解析（Chocolatey 安装不一定更新当前会话环境）

## 本地校验

- YAML 可被解析（ruby YAML）
- `find` 逻辑对「顶层 bin/」与「wrapper dir/bin/」两种布局都能定位 slangc
- `sele lint` 通过、`git diff --check` 干净

## 未验证

- 远端 CI 需要重跑确认；Windows 的 MSVC + C++20 modules（`import vulkan_hpp;`）在 Ninja 下尚未实测。
