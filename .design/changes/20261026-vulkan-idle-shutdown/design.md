# 20261026-vulkan-idle-shutdown

<!-- 为什么改、边界与不属于本次的范围 -->

## 退出时 Vulkan 资源在 pending 状态被销毁

### 现象

`ui_anim_demo` 正常关窗后输出大量校验错误：

```
vkFreeMemory(): can't be called on VkDeviceMemory ... currently in use by VkBuffer ... in use by VkCommandBuffer
vkDestroyFence(): ... currently in use by VkQueue
vkDestroySemaphore(): ... currently in use by VkQueue
vkFreeCommandBuffers(): pCommandBuffers[0] ... is in use
vkDestroySwapchainKHR(): ... currently in use by VkQueue
```

### 根因

`Renderer::~Renderer() {}` 是空析构。`swapchain`、vertex/uniform buffer 及其内存、
命令缓冲、fence/semaphore 都是 `Renderer` 的成员，在析构体之后按逆序释放；
此时 GPU 可能仍在执行上一帧提交，校验层因此判定「资源仍在使用」。

### 为什么之前没发现

所有冒烟都用 `timeout 5`（SIGTERM 强杀），进程来不及跑析构，校验层自然不会报。
这是测试盲区：**只有正常退出才会暴露资源释放问题**。

### 修复

1. `Renderer::~Renderer()` 开头调用 `context_.graphics_queue().waitIdle()`。
2. `ui_anim_demo` 与 `ui_gallery` 增加 `COSINE_EXIT_AFTER=<秒>` 钩子：
   到时 `glfwSetWindowShouldClose`，让进程走正常退出路径；CI 冒烟可以改用它，替代 `timeout` 强杀。

### 验证

- `COSINE_EXIT_AFTER=1 ./ui_anim_demo`：exit=0，`VUID-|validation (error|warning)` 计数 **0**。
- `COSINE_EXIT_AFTER=1 ./ui_gallery`：exit=0，计数 **0**。
- `ctest` 43/43（build 与 ASAN）。

### 边界

- 不做逐资源 fence 等待；退出路径整体 `queue.waitIdle()` 足够。
- 不改变运行时的每帧同步策略。
- 不处理 swapchain 重建路径（`recreate()` 内部已有 `waitIdle`）。
