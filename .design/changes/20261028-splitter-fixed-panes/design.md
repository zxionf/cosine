# 20261028-splitter-fixed-panes

<!-- 为什么改、边界与不属于本次的范围 -->

## 三栏尺寸完全独立：固定一侧像素

### 现象

拖树条时属性宽度也变；拖属性条时树宽也变。上一轮只把嵌套顺序改成「树 | (预览 | 属性)」，
解决了「拖属性条带动树」，但**拖树条仍会带动属性**（右区总宽变化，属性按比例跟着变）。

### 根因

`Splitter` 只有比例模式：外层总宽一变，内层按比例重新分配，另一侧必然跟着变。

### 修法

给 `Splitter` 增加「固定一侧像素宽度」：

- `set_fixed_first(px)` / `set_fixed_second(px)`（互斥，`-1` 回到比例模式）；
- `resolved_first()` 统一算第一块尺寸（固定优先，否则 `usable * ratio_`）；
- `apply_drag()`：固定模式改像素、比例模式改 `ratio_`；
- `append_to()` 的手柄位置也用 `resolved_first()`，否则手柄画在错误位置。

编辑器组装：

| splitter | 模式 | 效果 |
|---|---|---|
| `outer_splitter_`（树 \| 右区） | `set_fixed_first(kTreeWidth=280)` | 树宽固定，右区弹性 |
| `right_splitter_`（预览 \| 属性） | `set_fixed_second(kPropertyWidth=380)` | 属性宽固定，预览弹性 |

于是：拖树条只改树宽；拖属性条只改属性宽；窗口 resize 时**只有预览吸收**变化。

### 验证

- `ctest` **56/56**。
- `COSINE_EXIT_AFTER=3 ./cosine`：exit=0、validation=0。
- `COSINE_EXPORT_PNG=/tmp/ed_fix.png ./cosine`：树 280、属性 380、预览弹性，九行数值完整。

### 边界

- 时间轴仍按比例（`kTimelineRatio`），拖它不影响三栏。
- 预览时间标尺与逐帧未做。
