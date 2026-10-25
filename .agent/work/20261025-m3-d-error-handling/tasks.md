# 20261025-m3-d-error-handling 任务清单

## M3-D 错误处理

- [x] D01 `find_asset` / `require_asset` / AssetLocation 与错误文本
- [x] D02 字体加载错误上下文（CJK/coverage/default8/dynamic glyph）
- [x] D03 shader/文件读取错误包含路径与提示
- [x] D04 Vulkan/SwapChain 显式错误检查（设备、格式、image view）
- [x] D05 ui_demo 顶层异常处理
- [x] D06 `test_asset_path` + 全量回归

## 当前进度

- find_asset/require_asset 与 AssetLocation 已落地，缺失资源错误包含搜索路径和提示
- 字体/shader/Vulkan/SwapChain 错误补齐上下文；ui_demo 顶层 catch
- 13/13 ctest 通过；缺失 shader 手动验证输出 fatal [Asset]
