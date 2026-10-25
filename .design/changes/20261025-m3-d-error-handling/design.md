# 20261025-m3-d-error-handling

<!-- 为什么改、边界与不属于本次的范围 -->

## Asset 解析错误

- `find_asset()` 返回 `AssetLocation { path, found, searched }`，保留 `asset_path()` 兼容旧调用。
- 新增 `require_asset()`：文件缺失时抛出包含相对路径、所有搜索目录和修复提示的 `runtime_error`。
- 字体/shader/默认图集加载区分“找到路径”和“fallback 候选路径”，不再静默使用不存在的路径。

## 运行时错误上下文

- 资源、文件读取、字体、Vulkan 初始化、SwapChain 错误统一使用 `[Stage] detail` 格式。
- 字体构建/动态字形失败保留 warning，但包含字体路径、face index、coverage 文件。
- `read_file()` 错误包含完整文件路径和 shader/资源提示。
- Vulkan 设备/队列/surface 错误包含物理设备名或结果上下文。
- `SwapChain` 对空 format/present mode、image view 创建失败做显式检查，不再依赖 `assert` 或 `[0]`。

## 应用入口

- `examples/ui_demo` 的 main 增加顶层 try/catch，输出 `fatal: ...` 并返回非零。
- 不引入日志库、不改变异常层级；先用可读的 `runtime_error` 文本收口。

## 测试

- `test_asset_path` 覆盖找到/缺失/搜索目录和 `require_asset` 文本。
- 现有测试保持通过；Vulkan 错误通过手动/冒烟验证。
