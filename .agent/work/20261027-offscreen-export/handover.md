# 20261027-offscreen-export 交接

## 结果

`Renderer` 新增两个公开 API（`src/backend/vulkan/renderer.hpp`）：

```cpp
std::vector<unsigned char> render_offscreen(const graphics::Drawable& drawable,
                                            uint32_t width, uint32_t height,
                                            float ui_scale = 1.0f);
static bool write_png(const std::string& path, uint32_t width, uint32_t height,
                      const std::vector<unsigned char>& rgba);
```

配套重构：

- 抽出 `record_batches(command_buffer, target, scale)`，帧路径（`end()`）与离屏路径共用。
- `upload_vertex_data(command_buffer)` 与 `transition_image_layout(command_buffer, target, ...)`
  增加命令缓冲参数（原先硬编码 `command_buffers_[frame_index_]`）。
- `renderer.cpp` 增加 `#define STB_IMAGE_WRITE_IMPLEMENTATION`。

`ui_anim_showcase` 支持：

```bash
COSINE_EXPORT_PNG=/tmp/out.png ./build/examples/ui_anim_showcase
# exported /tmp/out.png
```

导出流程：`demo.update(1s)` → `uisys.tick` → 把 `root` 尺寸固定为 1200×800 并重排 →
`render_offscreen` → `write_png` → 退出。

## 三个坑（都踩过）

1. **字形同步顺序**：`append_to` 会按需加载新字号字形。离屏路径必须
   **先 `append_to`、再 `update_font_atlas_texture()`**。顺序反了会出现
   「按钮文字在、所有 Label 消失」（按钮用预填 16px，Label 用 12/13/15/18px）。
2. **格式必须与管线一致**：管线 color attachment 是 swapchain 的 `B8G8R8A8_SRGB`。
   用 `R8G8B8A8_SRGB` 会报 `VUID-vkCmdDrawIndexed-dynamicRenderingUnusedAttachments`。
3. **尺寸要显式对齐**：窗口 DPI 会改变 `root` 尺寸（本机 `framebuffer_scale = 1.5`，
   root 变成 1266×1582）。导出前必须 `set_size` + `layout()`。

## 验证

- `COSINE_EXPORT_PNG=/tmp/e4.png ./ui_anim_showcase`：exit=0、validation=0，1200×800 PNG。
  内容：三展区标题与标签、控制行按钮、进度条、时间与事件标签，位置与窗口一致。
- `ctest` **51/51**（build 与 ASAN）。
- `ui_gallery` / `ui_anim_showcase` 窗口路径冒烟：exit=0、validation=0（重构无回归）。
- `sele close` 为 `appended`（`DECISIONS #128`）；阶段计划 M6.0 已勾选，里程碑表 M6.0 标 ✅。

## 下一步：M6.1 项目序列化与撤销

- 二进制格式，魔术头 + 版本号；**字段表、端序、对齐在开工前先定**（`DECISIONS` 待追加）。
- 数据模型 `Project{ Scene, Timeline, metadata }`，只用稳定 id（`NodeId` / `PropertyId`）。
- round-trip 测试 + 撤销栈（命令接口 `apply` / `revert`）。
- 注意：`Scene` 的 `Node` 是单层结构 + 类型枚举，序列化友好；`Timeline` 的 `Track` 已按时间有序。
