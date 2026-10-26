# 20261026-atlas-space

<!-- 为什么改、边界与不属于本次的范围 -->

## M4.6 T1：图集空间治理

### 现象

多字号累积会挤满 2048² 图集。`add_glyph` 在空间不足时返回 `false`，
而 `ensure_glyph` 把这种失败**也**记入 `missing_`——该字形于是**永久不可用**，
即使之后图集腾出空间也不会重试。

### 修复

1. `add_glyph` 每次调用先重置 `last_add_out_of_space_`，空间不足时置为 `true`。
2. `ensure_glyph` 只在**非空间不足**的失败时写入 `missing_`；
   空间不足的字形保持「未缓存」，后续可重试。
3. 新增 `used_ratio()`（shelf packing 已用高度占比）与 `last_add_out_of_space()`，
   供监控、调试与测试。

### 验证

新建 `test/test_font_atlas_space.cpp`（4 个用例）：

- `used_ratio` 随添加增长，且始终在 `[0,1]`。
- 32×32 小图集放不下第二个 24×24 字形：失败、`last_add_out_of_space()` 为真、
  `has_glyph` 为假，**再次请求会重新调用 loader**（未被永久 caching）。
- 「字符不存在」导致的 loader 失败仍会缓存，不重复调用。
- 后续成功添加会重置空间标志（避免上一次失败污染下一次判断）。

### 边界（本次不做）

- 不做字形淘汰 / 图集重建（需要字符集清单与多图集支持，留待 T2 与 M6 资源管线）。
- 不做多纹理槽（渲染层当前只有 `TextureSlot::Texture` 单槽）。
- 不做字号档位化（会改变现有 UI 的字号精度）。
