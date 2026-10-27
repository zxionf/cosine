# 20261027-timeline-view 交接

## 结果（M6.3 第一阶段）

| 文件 | 内容 |
|---|---|
| `src/ui/timeline_view.hpp` | 标尺刻度、轨道行、关键帧菱形、播放头；点击 seek、拖动关键帧 |
| `src/anim/timeline.*` | `Track::set_key_time`（erase + 保序重插）、非 const `tracks()` |
| `src/cosine/editor.*` | 替换时间轴占位；播放头按 `time_` 循环；树行高 46 |
| `src/cosine/main.cpp` | `COSINE_EXPORT_PNG`（显式固定 1280×840） |
| `test/test_ui_timeline_view.cpp` | 4 组用例 |

## 三个坑

1. **导出时 `root` 尺寸为 0**：`UISystem::tick` 依赖窗口 framebuffer 回调，导出发生在首帧前 →
   `layout()` 里 `width <= 0` 直接 return，所有面板停在构造位置（左上角堆叠）。
   **修法**：导出分支显式 `set_size` + `layout()`，不调 `uisys.tick`。
2. **字号放大后行高没跟上**：`TreeView` 行高默认不随 `text_size` →
   文字被裁成半行。修法 `set_row_height(46)`。
3. **属性面板 9 行超出可用高度** → 换 `ScrollPanel`；时间轴 0s 关键帧压住轨道标签 → 轨道区留 8px。

## 顺带修的两个观察

- `editor.tick` 与 `uisys.tick` 都会动 root 尺寸，导出路径要避免两者打架。
- `grep -n` 的输出有时会被插件追加文本插进来，看起来像「文件某行含插件提示」；
  判断文件是否干净只认 `grep -c` + `tail | cat -A`（行尾 `$`）。

## 验证

- `test_ui_timeline_view`：`set_key_time` 保序与值保持、点击标尺 seek（含超界夹取）、
  拖动关键帧、点标签区不 seek。
- `ctest` **56/56**。
- `COSINE_EXPORT_PNG=/tmp/editor.png ./cosine`：1280×840，三栏 + 时间轴（0/1/2s 刻度、
  轨道标签、两个关键帧、播放头）正确，validation=0。
- `DECISIONS #134`；阶段计划 M6.3 标「进行中（一阶段完成）」。

## 下一步：M6.3 第二阶段

1. 框选多选（矩形框选 + 批量移动）。
2. 吸附（到整秒/到其它关键帧）、右键删除、插值切换（Linear/EaseIn/...）。
3. 接 `Player`：播放时游标跟随，点标尺 seek 驱动预览。
4. 验收：拖关键帧 → 存盘（`serialize`）→ 读回，轨道与关键帧一致。
