# 20261025-text-display-lab

<!-- 为什么改、边界与不属于本次的范围 -->

## 文字调试可视化

- 新增 `graphics::TextDebugOptions` 与 `append_text_debug()`：在文字之上叠加
  - 每个字形的包围盒（按 bearing/advance 计算，与绘制同源）
  - 整段文本的行框
  - 基线
  - advance 竖线（字距刻度）
- `Label` 增加 `set_text_debug(TextDebugOptions)`，与文本一起裁剪。
- 调试绘制取主题的强调色，可 `set_text_debug_color()` 覆盖。

## 文本显示选项

- `Label::set_text_align(TextAlign)`：Left / Center / Right，按控件宽度对齐整段文本。
- `Label::set_pixel_snap(bool)`：开启时把绘制原点对齐到整数像素，避免线性过滤导致的模糊（默认开启）。
- `Label::set_line_spacing(float)`：行间额外间距，用于多行排版（仅影响换行测量与绘制）。
- 以上选项不改变 `measure()` 的语义（除行距外），保证布局可预测。

## 排版组件

- `ui/typography.hpp`：
  - `TextRole`（Display / Title / Heading / Subheading / Body / Caption）
  - `text_style(role, theme)` 返回字号与颜色
  - `Heading`（按 level 生成 Title/Heading/Subheading）
  - `Paragraph`（默认换行 + 省略号可配 + 对齐）

## 优化点

- 像素对齐：绘制原点取整，减少半像素采样模糊。
- 复用 `FontAtlas` 现有 1px 字形内边距（线性采样不吸边）。
- 多行按行绘制，行高来自字体 metrics，避免叠加误差。
- 不在本次引入 SDF/MSDF、字距微调（kerning）与 shaping。

## 边界

- 不修改字体图集格式与生成流程。
- 不引入新的第三方排版库。
- `ui_demo` 由控件演示改为「文字实验台」，gallery 仍保留全部控件演示。
