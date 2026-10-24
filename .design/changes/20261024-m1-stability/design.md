# 20261024-m1-stability

本次变更执行 `.design/阶段计划.md` 的 M1：修正已知 P0/P1、收敛构建与资源路径、建立最小测试与 CI 脚本。不实现 GUI 控件、动画内核或 XEL 绑定。

## 实现约定

- 渲染批次携带纹理槽位（`WHITE` / `TEXTURE`）与显式初始化的 scissor；`Rectangle` / `Triangle` 使用 `uint32_t` 索引基址。
- GLFW user pointer 统一为 `Application*`；resize 经 `Application` 转发到 `Window`；`UISystem` 支持鼠标按下/抬起/点击与 `pressed_` 生命周期。
- `Widget` 使用非拥有 `parent` 指针；`add()` 设置 parent；布局脏标记沿 parent 链传播；`UIRoot` 是自由画布。
- `ScrollView` 用 owning children 暴露内容给命中测试，`set_content` 不再显式析构，滚轮方向与 offset clamp 明确。
- backend 收敛为 `cosine_backend` 静态库；资源统一放 `assets/`，运行时按 `COSINE_ASSET_DIR` → 可执行文件旁 `assets/` → 构建期注入目录解析。
- GLM 由 CMake 显式声明；STB 固定版本；slangc/glslc 使用 `find_program` 并在缺失时 fail fast；Wayland/X11 为 CMake option。
- 测试使用 CTest + 简单 assert；布局测试不依赖 Vulkan；`test_ui_layout` 与 3 个解释器测试注册为 ctest。
