# 20261028-splitter-panes

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.4b 第四阶段：可拖分区

### 交付

| 文件 | 内容 |
|---|---|
| `src/cosine/editor.*` | 嵌套三份 `Splitter`；`build_property_panel` 改为返回指针；`layout()` 只给分割器与顶栏定位 |
| 各 pane `min_size` | tree 160×160、preview 200×150、property 240×200、timeline 400×120 |

结构：

```
root_splitter_ (Vertical, ratio 0.72)
├── top_splitter_ (Horizontal, 0.70)
│    ├── left_splitter_ (Horizontal, 0.31)
│    │    ├── scene_tree_
│    │    └── preview_
│    └── property_panel_
└── timeline_view_
```

`Splitter::arrange` 直接给 pane 设 `{pos, size}`，比例区间由 pane 的 `min_size` 推导，
所以 pane 不再走 `place()`，只需给出合理下限。

### 关键坑（启动崩溃）

首帧 `Editor::layout()` 在 `root` 尺寸为 0 时 early return，`root_splitter_` 的 `min_size`
仍是默认 0；随后 `uisys.tick` 设置 root 尺寸并 layout →
`clamp_size(measure={0,0}, min={0,0})` 得到 0 尺寸 → 属性面板高 0 →
`ScrollView::thumb_length` 里 `std::clamp(val, 20, 0)` 触发 `hi < lo` 断言 → SIGABRT（exit 134）。

**修法**：组装时给 `root_splitter_` 一个非零下限 `{400, 300}`。

### 验证

- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0（修复前是 134）。
- `COSINE_EXPORT_PNG=/tmp/ed_sp.png ./cosine`：三栏 + 底部全宽时间轴 + 可见分割条。

### 边界

- 预览时间标尺与逐帧未做。
- 框选追加（Shift/Ctrl）、批量拖动合成单条命令未做。
- 场景树下半部分背景与面板底色不一致（`TreeView` 只填充内容区）。
