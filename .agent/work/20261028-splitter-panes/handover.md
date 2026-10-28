# 20261028-splitter-panes 交接

## 结果（M6.4b 第四阶段）

| 文件 | 内容 |
|---|---|
| `src/cosine/editor.*` | 三份嵌套 `Splitter`；`build_property_panel` 返回指针；`layout()` 只定位分割器与顶栏 |
| pane `min_size` | tree 160×160、preview 200×150、property 240×200、timeline 400×120 |

```
root_splitter_ (Vertical, 0.72)
├── top_splitter_ (Horizontal, 0.70)
│    ├── left_splitter_ (Horizontal, 0.31)
│    │    ├── scene_tree_
│    │    └── preview_
│    └── property_panel_
└── timeline_view_
```

## 关键坑：启动即 SIGABRT（exit 134）

现象：

```
/usr/include/c++/16/bits/stl_algo.h:3616: std::clamp<float>: Assertion '!(__hi < __lo)' failed.
#5  xel::ui::ScrollView::thumb_length (scroll_view.cpp:131)
```

链路：

1. 首帧 `Editor::layout()` 看到 `root` 尺寸为 0 → **early return**；
2. `uisys.tick` 随后给 root 设尺寸并 `root_->layout()`；
3. `layout_child` 用 `clamp_size(measure={0,0}, min_size={0,0})` → **0 尺寸**；
4. `Splitter::arrange` 把 0 高度传给属性面板 → 内部 `ScrollView::global_.size.y == 0`；
5. `thumb_length` 里 `std::clamp(track * ratio, scrollbar_min_thumb=20, track=0)` →
   `hi < lo` → 断言 → `abort()`。

**修法**：`build_panels` 组装后给 `root_splitter_` 一个非零下限 `set_min_size({400, 300})`，
首帧即使 root 还没尺寸也不会被 clamp 到 0。

## 验证

- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0（修复前 134）。
- `COSINE_EXPORT_PNG=/tmp/ed_sp.png ./cosine`：三栏 + 底部全宽时间轴 + 可见分割条。
- `sele lint` 四项 ok；`DECISIONS #140`。

## 剩余（M6.4b 收尾）

1. 预览时间标尺与逐帧。
2. 框选追加（Shift/Ctrl）、批量拖动合成单条命令。
3. 场景树下半部分背景色与面板底色不一致（`TreeView` 只填充内容区，可在 `append_to` 里补背景）。
