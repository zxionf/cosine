# 20261027-timeline-view

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.3 第一阶段：时间轴编辑视图

### 交付

| 文件 | 内容 |
|---|---|
| `src/ui/timeline_view.hpp` | 标尺 + 轨道行 + 关键帧菱形 + 播放头；点击 seek、拖动关键帧 |
| `src/anim/timeline.*` | `Track::set_key_time`（保序重插）、非 const `tracks()` |
| `src/cosine/editor.*` | 用 `TimelineView` 替换占位；播放头按 `time_` 循环推进 |
| `src/cosine/main.cpp` | `COSINE_EXPORT_PNG`（与 showcase 同机制，显式固定导出尺寸） |
| `test/test_ui_timeline_view.cpp` | 4 组用例 |

### 时间轴细节

- 左侧标签列 160px（`node <id> · prop <id>`），轨道区再留 8px 边距，避免 0s 关键帧压住标签。
- 刻度步长从 `{0.1,0.2,0.5,1,2,5,10,30}` 里选第一个满足「≥64px」的。
- 点击关键帧（±8px）选中并进入拖拽；拖拽调 `Track::set_key_time`（内部 erase + 保序重插）。
- 点击其它位置 = seek 播放头，夹在 `[0, duration]`。

### 顺带修的布局问题（离屏导出时暴露）

1. **树行高没跟字号**：编辑器字号 30，`TreeView` 默认行高把文字裁掉 → `set_row_height(46)`。
2. **属性面板 9 行溢出**：`Panel` → `ScrollPanel`，超出可滚动。
3. **0s 关键帧与轨道标签重叠** → 轨道区加 8px 左内边距。

### 踩到的坑

**导出时 `root` 尺寸为 0**：`UISystem::tick` 会在窗口收到 framebuffer 回调后才设置 root 尺寸；
导出发生在首帧之前，`layout()` 里 `width <= 0` 直接 return，所有面板停在构造位置（左上角堆叠）。
修法：导出分支**显式** `set_size` + `layout()`，且不调用 `uisys.tick`。

### 验证

- `test_ui_timeline_view`：`set_key_time` 保序与值保持、点击标尺 seek（含超界夹取）、拖动关键帧、点标签区不 seek。
- `ctest` **56/56**。
- `COSINE_EXPORT_PNG=/tmp/editor.png ./cosine`：1280×840；三栏 + 时间轴（0/1/2s 刻度、轨道标签、两个关键帧、播放头）正确，validation=0。

### 边界（下一阶段）

- 未做框选多选、吸附、右键删除、插值切换。
- 播放头由 `time_` 模拟，尚未接 `Player`；预览不受播放头影响。
- 时间轴缩放未做交互（只有 `set_pixels_per_second` API）。
