# 20261026-atlas-space 交接

## 结果

- `graphics/FontAtlas`：
  - `add_glyph` 每次调用重置 `last_add_out_of_space_`，空间不足时置真；
  - `ensure_glyph` **只在非空间不足**的失败时写入 `missing_`——空间不足的字形可重试；
  - 新增 `used_ratio()`（shelf packing 已用高度占比）与 `last_add_out_of_space()`。
- 新增 `test/test_font_atlas_space.cpp`（4 个用例），注册为 `font_atlas_space`。

## 验证

- `ctest` **48/48**（build 与 ASAN）。
- 用例覆盖：`used_ratio` 增长、空间不足可重试、缺字仍缓存、标志复位。
- `sele close` 为 `appended`（`DECISIONS #121`）。
- 阶段计划 M4.6 的 T1 已勾选。

## 修复的真实 bug

之前 `ensure_glyph` 把「图集满」也写进 `missing_`，导致该字形**永久不可用**，
即使后来图集腾出空间也不会重试。现在只有「字符不存在」才永久缓存。

## 未做（留待后续）

- **T2**：纹理上传优化（dirty-rect / 分页图集），解决每次新增字形重传 2048² 的卡顿。
- **字形淘汰 / 图集重建**：需要字符集清单与多图集支持。
- **多纹理槽**：渲染层当前只有 `TextureSlot::Texture` 单槽。
- **字号档位化**：会改变现有 UI 字号精度，暂不做。

## 排查建议

- 如果出现「某字号文字模糊」，先看 `atlas.used_ratio()` 是否接近 1——
  那是回退到 16px 基准缩放的表现，说明图集已满。
- `last_add_out_of_space()` 可用于区分「缺字」与「空间不足」。
