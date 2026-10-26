# 20261026-vulkan-idle-shutdown 交接

## 结果

- `Renderer::~Renderer()` 开头调用 `context_.graphics_queue().waitIdle()`。
- `ui_anim_demo` / `ui_gallery` 增加 `COSINE_EXIT_AFTER=<秒>`：到时正常关窗，
  使进程走析构路径（替代 `timeout` 强杀）。

## 验证

- `COSINE_EXIT_AFTER=1 ./ui_anim_demo`：exit=0，`VUID-|validation (error|warning)` = 0。
- `COSINE_EXIT_AFTER=1 ./ui_gallery`：同上。
- `ctest` 43/43（build 与 ASAN）。

## 根因笔记

`Renderer` 的成员（swapchain、buffer 内存、命令缓冲、fence/semaphore）在析构体之后才释放，
空析构没等 GPU 完成，校验层报「in use by VkQueue / VkCommandBuffer」。

**测试盲区**：以前所有冒烟都是 `timeout N ./app`（SIGTERM），进程来不及析构，
所以从未暴露。**以后冒烟优先用 `COSINE_EXIT_AFTER`，timeout 只作为兜底。**

## 冒烟命令（新）

```bash
cd build/examples
COSINE_EXIT_AFTER=1 ./ui_anim_demo 2>&1 | grep -E "VUID-|validation (error|warning)"   # 应为空
COSINE_EXIT_AFTER=1 ./ui_gallery  2>&1 | grep -E "VUID-|validation (error|warning)"   # 应为空
```

## 下一步

M4 `property-system`：`PropertyId` 注册表 + `variant` 类型擦除读写。
