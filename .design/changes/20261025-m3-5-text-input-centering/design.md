# 20261025-m3-5-text-input-centering

<!-- 为什么改、边界与不属于本次的范围 -->

## 单行 TextInput 文字偏下

**根因**：上一轮把垂直居中基准从「字号」改成「行高」，但同时写了

```cpp
text_y = y + std::max(pad, (height - line_height) * 0.5f);
```

当行高（≈23）小于控件高度（30）时，`(30-23)/2 = 3.5 < padding(8)`，
`max()` 取了 padding，文字被推到 `y + 8`，整体偏下 4~5px。

多行输入不受影响，因为多行本来就从 `padding` 开始排版。

**修法**：

```cpp
const float line_h = line_height();
const float text_y = y + std::max(0.0f, (height - line_h) * 0.5f);
```

同时把单行的选区高度改为 `line_h`、光标在行盒内居中（高度仍为字号）。

## 测试

`test_ui_text_input` 新增 `test_single_line_vertical_centering`：

- 用合成图集（行高 1.5em）构造「行高 < 控件高度」的场景
- 断言文字四边形中心与控件中心相差 ≤ 2px
- 断言上/下留白相差 ≤ 4px
- 反向验证：恢复旧实现（`max(pad, ...)`）该用例立即失败

## 边界

- 不改多行输入布局；不引入基线对齐的自定义策略。
