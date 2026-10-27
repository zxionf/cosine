# 20261027-line-height-metrics 交接

## 结果

- `graphics/FontAtlas` 新增 `static Metrics normalize_metrics(Metrics)`：
  `line_height_ratio = max(line_height_ratio, ascent_ratio - descent_ratio, 1.0)`。
  （`descent_ratio` 按 FreeType 约定为负，所以是减号。）
- `begin_generated()` 存入 metrics 前归一化。
- `FontFace::metrics()` 改为 `return FontAtlas::normalize_metrics(metrics);`（去掉本地 clamp）。
- 新增 `test/test_font_atlas_metrics.cpp`（4 个用例）。

## 验证

- `ctest` **50/50**（build 与 ASAN）。
- `ui_demo`（XEL Text Lab）目视：`COSINE_TEXT_DEBUG=1` 打开行框后，
  96px 的 “Hqyp gjpqy” 行框完整包住 q/y/p/j/g 的降部。
- `sele close` 为 `appended`（`DECISIONS #124`）；阶段计划 M4.6 的 T4 已勾选。

## 用户验证方法

```bash
cd build/examples
COSINE_TEXT_DEBUG=1 ./ui_demo      # 行框 / 基线 / advance / 字形框全开
./ui_demo                          # 默认关闭，F3 单独切行框
```

- **F1** overlay、**F2** 字形框、**F3** 行框（同时切基线）、**F4** advance、**F5** 像素对齐。
- 重点看 **“Descenders at large size”** 这一块的 96px 大字号。

## 预期变化（不是回归）

行高可能变大（取决于字体），因此多行段落、TextInput、列表行高会略微增高。
这正是 T4 的目标：行框必须容得下降部。

## 后续

- **T5 字距与排版细节**（kerning / letter spacing / 避头尾 / 中英混排间距）。
- 或切回 **M5 XEL 绑定**。
